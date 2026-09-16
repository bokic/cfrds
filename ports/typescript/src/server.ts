import {
  CFRDS_DEBUGGER_EVENT_TYPE,
  CFRDS_STATUS,
  CFRDSError,
  CFRDSResponseError,
  CFRDSCommandError,
  CFRDSValidationError,
  ServerContext,
  BrowseDirItem,
  FileContent,
  SqlTableInfoItem,
  SqlColumnInfoItem,
  SqlPrimaryKeyItem,
  SqlForeignKeyItem,
  SqlResultSet,
  SqlMetadataItem,
  DebuggerEvent,
  SecurityAnalyzerResult,
  AdminApiCustomTagPaths,
  AdminApiMappings,
  IdeDefaultResult,
  SecurityAnalyzerStatus,
} from "./types";
import { encodePassword, parseNumber, parseString, parseBytearray, parseTimestamp, parseStringListItem, wddxDeserialize, wddxGet, wddxGetString, wddxGetNumber, parseXml, parseWddxNode, XmlNode, safeInt } from "./parser";
import { sendRdsCommand } from "./transport";
import { VERSION } from "./version";
import * as http from "http";

function escapeXml(str: string): string {
  return str
    .replace(/&/g, "&amp;")
    .replace(/</g, "&lt;")
    .replace(/>/g, "&gt;")
    .replace(/"/g, "&quot;")
    .replace(/'/g, "&apos;");
}

/**
 * Mirrors the C library's `cfrds_buffer_append_escaped`: backslash-escapes the
 * `:` and `;` delimiters used by the admin API mapping argument syntax.
 */
function escapeMappingValue(str: string): string {
  let out = "";
  for (const ch of str) {
    if (ch === ":" || ch === ";") {
      out += "\\";
    }
    out += ch;
  }
  return out;
}

const FILE_NOT_FOUND_PREFIX = "The system cannot find the path specified: ";

export function cfrds_version(): string {
  return VERSION;
}

export function cfrds_version_major(): number {
  const parts = VERSION.split(".");
  return parseInt(parts[0], 10) || 0;
}

export function cfrds_version_minor(): number {
  const parts = VERSION.split(".");
  return parseInt(parts[1], 10) || 0;
}

export function cfrds_version_patch(): number {
  const parts = VERSION.split(".");
  if (parts.length >= 3) {
    const patchStr = parts[2].split(/[-+]/)[0];
    return parseInt(patchStr, 10) || 0;
  }
  return 0;
}

export function cfrds_version_int(): number {
  const major = cfrds_version_major();
  const minor = cfrds_version_minor();
  const patch = cfrds_version_patch();
  return major * 10000 + minor * 100 + patch;
}

export class Server {
  private ctx: ServerContext;
  private lastError: CFRDSError | null = null;

  constructor(
    hostname: string = "127.0.0.1",
    port: number = 8500,
    username: string = "admin",
    password: string = ""
  ) {
    this.ctx = {
      config: { host: hostname, port, username, password },
      encodedPassword: encodePassword(password),
      agent: new http.Agent({ keepAlive: true }),
    };
  }

  getHost(): string { return this.ctx.config.host; }
  getPort(): number { return this.ctx.config.port; }
  getUsername(): string { return this.ctx.config.username; }
  getPassword(): string { return this.ctx.config.password; }

  /**
   * Returns the last error raised by a command, mirroring `cfrds_server_get_error`.
   * Unlike the C library, commands still reject with the error; this is a record
   * of the most recent failure.
   */
  getError(): CFRDSError | null {
    return this.lastError;
  }

  /** Clears the recorded last error, mirroring `cfrds_server_clear_error`. */
  clearError(): void {
    this.lastError = null;
  }

  /**
   * Sends an RDS command, recording any CFRDS error as the server's last error
   * (the C library clears the error before each command and rethrows/stores it).
   */
  private async send(command: string, args: (string | Buffer)[]): Promise<Buffer> {
    this.lastError = null;
    try {
      return await sendRdsCommand(this.ctx, command, args);
    } catch (e) {
      const err =
        e instanceof CFRDSError
          ? e
          : new CFRDSError(e instanceof Error ? e.message : String(e));
      this.lastError = err;
      throw e;
    }
  }

  async close(): Promise<void> {
    if (this.ctx.agent) {
      this.ctx.agent.destroy();
      this.ctx.agent = undefined;
    }
  }

  // Browse Directory
  async browseDir(path: string): Promise<BrowseDirItem[]> {
    if (path === null || path === undefined) {
      throw new CFRDSValidationError("path is required", CFRDS_STATUS.PARAM_IS_NULL);
    }
    const raw = await this.send("BROWSEDIR", [path, ""]);
    const [total, offset] = parseNumber(raw, 0);
    if (total < 0 || (total !== 0 && total % 5 !== 0)) {
      throw new CFRDSResponseError(
        "Invalid total items count in browseDir response",
        CFRDS_STATUS.RESPONSE_ERROR
      );
    }
    const cnt = total / 5;
    const items: BrowseDirItem[] = [];
    let off = offset;
    for (let i = 0; i < cnt; i++) {
      const [strKind, o1] = parseString(raw, off);
      const [filename, o2] = parseString(raw, o1);
      const [strPerms, o3] = parseString(raw, o2);
      const [strSize, o4] = parseString(raw, o3);
      const [strTs, o5] = parseString(raw, o4);
      off = o5;

      const kind = (strKind === "D:" || strKind === "D") ? "D" : "F";
      const permsNum = strPerms ? parseInt(strPerms, 10) : 0;
      const size = strSize ? parseInt(strSize, 10) : 0;

      const modified = parseTimestamp(strTs);

      // Map permission flags from C source's cfrds_browse_dir_item_get_permissions semantics:
      // - 0x01: Read-only (R) -> maps to FILE_ATTRIBUTE_READONLY (1)
      // - 0x02: Hidden (H)    -> maps to FILE_ATTRIBUTE_HIDDEN (2)
      // - 0x04: System (S)    -> maps to FILE_ATTRIBUTE_SYSTEM (4)
      // - 0x10: Directory (D) -> maps to FILE_ATTRIBUTE_DIRECTORY (16)
      // - 0x20: Archive (A)   -> maps to FILE_ATTRIBUTE_ARCHIVE (32)
      // - 0x80: Normal (N)    -> maps to FILE_ATTRIBUTE_NORMAL (128)
      const permissions = ((permsNum & 0x10) || kind === "D" ? "D" : "-") +
        ((permsNum & 0x01) ? "R" : "-") +
        ((permsNum & 0x02) ? "H" : "-") +
        ((permsNum & 0x04) ? "S" : "-") +
        ((permsNum & 0x20) ? "A" : "-") +
        ((permsNum & 0x80) ? "N" : "-");

      items.push({
        kind,
        name: filename,
        permissions,
        size,
        modified,
      });
    }
    return items;
  }

  // File Operations
  async fileRead(filepath: string): Promise<FileContent> {
    if (filepath === null || filepath === undefined) {
      throw new CFRDSValidationError("filepath is required", CFRDS_STATUS.PARAM_IS_NULL);
    }
    const raw = await this.send("FILEIO", [filepath, "READ", ""]);
    const [, offset] = parseNumber(raw, 0);
    const [dataBytes, o1] = parseBytearray(raw, offset);
    const [modifiedStr, o2] = parseString(raw, o1);
    const [permission] = parseString(raw, o2);
    return { data: dataBytes, modified: parseTimestamp(modifiedStr), permission };
  }

  async fileWrite(filepath: string, content: string | Buffer): Promise<void> {
    if (filepath === null || filepath === undefined) {
      throw new CFRDSValidationError("filepath is required", CFRDS_STATUS.PARAM_IS_NULL);
    }
    if (content === null || content === undefined) {
      throw new CFRDSValidationError("content is required", CFRDS_STATUS.PARAM_IS_NULL);
    }
    const data = typeof content === "string" ? Buffer.from(content, "utf-8") : content;
    await this.send("FILEIO", [filepath, "WRITE", "", data]);
  }

  async fileRename(filepathFrom: string, filepathTo: string): Promise<void> {
    if (filepathFrom === null || filepathFrom === undefined) {
      throw new CFRDSValidationError("filepathFrom is required", CFRDS_STATUS.PARAM_IS_NULL);
    }
    if (filepathTo === null || filepathTo === undefined) {
      throw new CFRDSValidationError("filepathTo is required", CFRDS_STATUS.PARAM_IS_NULL);
    }
    await this.send("FILEIO", [filepathFrom, "RENAME", "", filepathTo]);
  }

  async fileRemove(filepath: string): Promise<void> {
    if (filepath === null || filepath === undefined) {
      throw new CFRDSValidationError("filepath is required", CFRDS_STATUS.PARAM_IS_NULL);
    }
    await this.send("FILEIO", [filepath, "REMOVE", "", "F"]);
  }

  async dirRemove(dirpath: string): Promise<void> {
    if (dirpath === null || dirpath === undefined) {
      throw new CFRDSValidationError("dirpath is required", CFRDS_STATUS.PARAM_IS_NULL);
    }
    await this.send("FILEIO", [dirpath, "REMOVE", "", "D"]);
  }

  async fileExists(pathname: string): Promise<boolean> {
    if (pathname === null || pathname === undefined) {
      throw new CFRDSValidationError("pathname is required", CFRDS_STATUS.PARAM_IS_NULL);
    }
    try {
      await this.send("FILEIO", [pathname, "EXISTENCE", "", ""]);
      return true;
    } catch (e) {
      // Only the exact "not found" command failure means the path does not
      // exist; every other failure (permissions, connectivity, ...) must
      // propagate just like the C implementation.
      if (e instanceof CFRDSCommandError) {
        const serverMessage = e.message.replace(/^COMMAND_FAILED:\s?/, "");
        if (serverMessage.startsWith(FILE_NOT_FOUND_PREFIX)) {
          return false;
        }
      }
      throw e;
    }
  }

  async dirCreate(dirpath: string): Promise<void> {
    if (dirpath === null || dirpath === undefined) {
      throw new CFRDSValidationError("dirpath is required", CFRDS_STATUS.PARAM_IS_NULL);
    }
    await this.send("FILEIO", [dirpath, "CREATE", "", ""]);
  }

  async cfRootDir(): Promise<string> {
    const raw = await this.send("FILEIO", ["", "CF_DIRECTORY"]);
    const [, offset] = parseNumber(raw, 0);
    const [pathStr] = parseString(raw, offset);
    return pathStr;
  }

  // SQL Operations
  async sqlDsninfo(): Promise<string[]> {
    const raw = await this.send("DBFUNCS", ["", "DSNINFO"]);
    const [cnt, offset] = parseNumber(raw, 0);
    const dsns: string[] = [];
    let off = offset;
    for (let i = 0; i < cnt; i++) {
      const [item, o] = parseString(raw, off);
      off = o;
      const fields = parseStringListItem(item);
      const name = fields[0] || item;
      dsns.push(name);
    }
    return dsns;
  }

  async sqlTableinfo(connectionName: string): Promise<SqlTableInfoItem[]> {
    if (connectionName === null || connectionName === undefined) {
      throw new CFRDSValidationError("connectionName is required", CFRDS_STATUS.PARAM_IS_NULL);
    }
    const raw = await this.send("DBFUNCS", [connectionName, "TABLEINFO"]);
    const [cnt, offset] = parseNumber(raw, 0);
    const tables: SqlTableInfoItem[] = [];
    let off = offset;
    for (let i = 0; i < cnt; i++) {
      const [item, o] = parseString(raw, off);
      off = o;
      const fields = parseStringListItem(item);
      tables.push({
        unknown: fields[0] || "",
        schema: fields[1] || "",
        name: fields[2] || "",
        type: fields[3] || "",
      });
    }
    return tables;
  }

  async sqlColumninfo(connectionName: string, tableName: string): Promise<SqlColumnInfoItem[]> {
    if (connectionName === null || connectionName === undefined) {
      throw new CFRDSValidationError("connectionName is required", CFRDS_STATUS.PARAM_IS_NULL);
    }
    if (tableName === null || tableName === undefined) {
      throw new CFRDSValidationError("tableName is required", CFRDS_STATUS.PARAM_IS_NULL);
    }
    const raw = await this.send("DBFUNCS", [connectionName, "COLUMNINFO", tableName]);
    const [cnt, offset] = parseNumber(raw, 0);
    const cols: SqlColumnInfoItem[] = [];
    let off = offset;
    for (let i = 0; i < cnt; i++) {
      const [item, o] = parseString(raw, off);
      off = o;
      const fields = parseStringListItem(item);
      cols.push({
        schema: fields[0] || "",
        owner: fields[1] || "",
        table: fields[2] || "",
        name: fields[3] || "",
        type: safeInt(fields[4]),
        typeStr: fields[5] || "",
        precision: safeInt(fields[6]),
        length: safeInt(fields[7]),
        scale: safeInt(fields[8]),
        radix: safeInt(fields[9]),
        nullable: safeInt(fields[10]),
      });
    }
    return cols;
  }

  async sqlPrimarykeys(connectionName: string, tableName: string): Promise<SqlPrimaryKeyItem[]> {
    if (connectionName === null || connectionName === undefined) {
      throw new CFRDSValidationError("connectionName is required", CFRDS_STATUS.PARAM_IS_NULL);
    }
    if (tableName === null || tableName === undefined) {
      throw new CFRDSValidationError("tableName is required", CFRDS_STATUS.PARAM_IS_NULL);
    }
    const raw = await this.send("DBFUNCS", [connectionName, "PRIMARYKEYS", tableName]);
    const [cnt, offset] = parseNumber(raw, 0);
    const keys: SqlPrimaryKeyItem[] = [];
    let off = offset;
    for (let i = 0; i < cnt; i++) {
      const [item, o] = parseString(raw, off);
      off = o;
      const fields = parseStringListItem(item);
      keys.push({
        catalog: fields[0] || "",
        owner: fields[1] || "",
        table: fields[2] || "",
        column: fields[3] || "",
        key_sequence: safeInt(fields[4]),
      });
    }
    return keys;
  }

  async sqlForeignkeys(connectionName: string, tableName: string): Promise<SqlForeignKeyItem[]> {
    if (connectionName === null || connectionName === undefined) {
      throw new CFRDSValidationError("connectionName is required", CFRDS_STATUS.PARAM_IS_NULL);
    }
    if (tableName === null || tableName === undefined) {
      throw new CFRDSValidationError("tableName is required", CFRDS_STATUS.PARAM_IS_NULL);
    }
    const raw = await this.send("DBFUNCS", [connectionName, "FOREIGNKEYS", tableName]);
    const [cnt, offset] = parseNumber(raw, 0);
    const keys: SqlForeignKeyItem[] = [];
    let off = offset;
    for (let i = 0; i < cnt; i++) {
      const [item, o] = parseString(raw, off);
      off = o;
      const fields = parseStringListItem(item);
      keys.push({
        pkcatalog: fields[0] || "",
        pkowner: fields[1] || "",
        pktable: fields[2] || "",
        pkcolumn: fields[3] || "",
        fkcatalog: fields[4] || "",
        fkowner: fields[5] || "",
        fktable: fields[6] || "",
        fkcolumn: fields[7] || "",
        key_sequence: safeInt(fields[8]),
        updaterule: safeInt(fields[9]),
        deleterule: safeInt(fields[10]),
      });
    }
    return keys;
  }

  async sqlImportedkeys(connectionName: string, tableName: string): Promise<SqlForeignKeyItem[]> {
    if (connectionName === null || connectionName === undefined) {
      throw new CFRDSValidationError("connectionName is required", CFRDS_STATUS.PARAM_IS_NULL);
    }
    if (tableName === null || tableName === undefined) {
      throw new CFRDSValidationError("tableName is required", CFRDS_STATUS.PARAM_IS_NULL);
    }
    const raw = await this.send("DBFUNCS", [connectionName, "IMPORTEDKEYS", tableName]);
    const [cnt, offset] = parseNumber(raw, 0);
    const keys: SqlForeignKeyItem[] = [];
    let off = offset;
    for (let i = 0; i < cnt; i++) {
      const [item, o] = parseString(raw, off);
      off = o;
      const fields = parseStringListItem(item);
      keys.push({
        pkcatalog: fields[0] || "",
        pkowner: fields[1] || "",
        pktable: fields[2] || "",
        pkcolumn: fields[3] || "",
        fkcatalog: fields[4] || "",
        fkowner: fields[5] || "",
        fktable: fields[6] || "",
        fkcolumn: fields[7] || "",
        key_sequence: safeInt(fields[8]),
        updaterule: safeInt(fields[9]),
        deleterule: safeInt(fields[10]),
      });
    }
    return keys;
  }

  async sqlExportedkeys(connectionName: string, tableName: string): Promise<SqlForeignKeyItem[]> {
    if (connectionName === null || connectionName === undefined) {
      throw new CFRDSValidationError("connectionName is required", CFRDS_STATUS.PARAM_IS_NULL);
    }
    if (tableName === null || tableName === undefined) {
      throw new CFRDSValidationError("tableName is required", CFRDS_STATUS.PARAM_IS_NULL);
    }
    const raw = await this.send("DBFUNCS", [connectionName, "EXPORTEDKEYS", tableName]);
    const [cnt, offset] = parseNumber(raw, 0);
    const keys: SqlForeignKeyItem[] = [];
    let off = offset;
    for (let i = 0; i < cnt; i++) {
      const [item, o] = parseString(raw, off);
      off = o;
      const fields = parseStringListItem(item);
      keys.push({
        pkcatalog: fields[0] || "",
        pkowner: fields[1] || "",
        pktable: fields[2] || "",
        pkcolumn: fields[3] || "",
        fkcatalog: fields[4] || "",
        fkowner: fields[5] || "",
        fktable: fields[6] || "",
        fkcolumn: fields[7] || "",
        key_sequence: safeInt(fields[8]),
        updaterule: safeInt(fields[9]),
        deleterule: safeInt(fields[10]),
      });
    }
    return keys;
  }

  async sqlSqlstmnt(connectionName: string, sql: string): Promise<SqlResultSet> {
    if (connectionName === null || connectionName === undefined) {
      throw new CFRDSValidationError("connectionName is required", CFRDS_STATUS.PARAM_IS_NULL);
    }
    if (sql === null || sql === undefined) {
      throw new CFRDSValidationError("sql is required", CFRDS_STATUS.PARAM_IS_NULL);
    }
    const raw = await this.send("DBFUNCS", [connectionName, "SQLSTMNT", sql]);
    const [cnt, offset] = parseNumber(raw, 0);
    const rows = Math.max(0, cnt - 1);
    if (cnt <= 0) {
      return { columns: 0, rows: 0, names: [], values: [] };
    }

    let off = offset;
    const [colStr, o1] = parseString(raw, off);
    off = o1;
    const names = parseStringListItem(colStr);
    const cols = names.length;

    const dataRows: (string | null)[][] = [];
    for (let i = 0; i < rows; i++) {
      const [rStr, o] = parseString(raw, off);
      off = o;
      dataRows.push(parseStringListItem(rStr));
    }

    return { columns: cols, rows, names, values: dataRows };
  }

  async sqlMetadata(connectionName: string, sql: string): Promise<SqlMetadataItem[]> {
    if (connectionName === null || connectionName === undefined) {
      throw new CFRDSValidationError("connectionName is required", CFRDS_STATUS.PARAM_IS_NULL);
    }
    if (sql === null || sql === undefined) {
      throw new CFRDSValidationError("sql is required", CFRDS_STATUS.PARAM_IS_NULL);
    }
    const raw = await this.send("DBFUNCS", [connectionName, "SQLMETADATA", sql]);
    const [cnt, offset] = parseNumber(raw, 0);
    const meta: SqlMetadataItem[] = [];
    let off = offset;
    for (let i = 0; i < cnt; i++) {
      const [item, o] = parseString(raw, off);
      off = o;
      const fields = parseStringListItem(item);
      meta.push({
        name: fields[0] || "",
        type: fields[1] || "",
        jtype: fields[2] || "",
      });
    }
    return meta;
  }

  async sqlGetsupportedcommands(): Promise<string[]> {
    const raw = await this.send("DBFUNCS", ["", "SUPPORTEDCOMMANDS"]);
    const [cnt, offset] = parseNumber(raw, 0);
    const cmds: string[] = [];
    let off = offset;
    for (let i = 0; i < cnt; i++) {
      const [cmdStr, o] = parseString(raw, off);
      off = o;
      cmds.push(...parseStringListItem(cmdStr));
    }
    return cmds;
  }

  async sqlDbdescription(connectionName: string): Promise<string> {
    if (connectionName === null || connectionName === undefined) {
      throw new CFRDSValidationError("connectionName is required", CFRDS_STATUS.PARAM_IS_NULL);
    }
    const raw = await this.send("DBFUNCS", [connectionName, "DBDESCRIPTION"]);
    const [, offset] = parseNumber(raw, 0);
    const [item] = parseString(raw, offset);
    const fields = parseStringListItem(item);
    return fields[0] || item;
  }

  // Debugger Operations
  async debuggerStart(): Promise<string> {
    // The C client sends a WDDX packet with REMOTE_SESSION=true as the second
    // argument, not an empty string.
    const wddx =
      "<wddxPacket version='1.0'><header/><data><array length='1'><struct type='java.util.HashMap'><var name='REMOTE_SESSION'><boolean value='true'/></var></struct></array></data></wddxPacket>";
    const raw = await this.send("DBGREQUEST", ["DBG_START", wddx]);
    const [count, offset] = parseNumber(raw, 0);
    if (count !== 2) {
      throw new CFRDSResponseError("Invalid debuggerStart response count", CFRDS_STATUS.RESPONSE_ERROR);
    }
    const [sessionId] = parseString(raw, offset);
    if (!sessionId) {
      throw new CFRDSResponseError("Empty debugger session id", CFRDS_STATUS.RESPONSE_ERROR);
    }
    return sessionId;
  }

  async debuggerStop(sessionName: string): Promise<void> {
    if (sessionName === null || sessionName === undefined) {
      throw new CFRDSValidationError("sessionName is required", CFRDS_STATUS.PARAM_IS_NULL);
    }
    await this.send("DBGREQUEST", ["DBG_STOP", sessionName]);
  }

  async debuggerServerStop(sessionName: string): Promise<void> {
    if (sessionName === null || sessionName === undefined) {
      throw new CFRDSValidationError("sessionName is required", CFRDS_STATUS.PARAM_IS_NULL);
    }
    await this.send("DBGREQUEST", ["DBG_SERVER_STOP", sessionName]);
  }

  async debuggerGetServerInfo(sessionName: string): Promise<number> {
    if (sessionName === null || sessionName === undefined) {
      throw new CFRDSValidationError("sessionName is required", CFRDS_STATUS.PARAM_IS_NULL);
    }
    const raw = await this.send("DBGREQUEST", ["DBG_GET_DEBUG_SERVER_INFO", sessionName]);
    const [count, offset] = parseNumber(raw, 0);
    if (count !== 1) {
      throw new CFRDSResponseError("Invalid debuggerGetServerInfo response count", CFRDS_STATUS.RESPONSE_ERROR);
    }
    const [wddxXml] = parseString(raw, offset);
    const parsed = wddxDeserialize(wddxXml);
    // C reads "0,STATUS" and requires "RDS_OK", then reads "0,DEBUG_SERVER_PORT"
    // from within that same array element.
    if (wddxGetString(parsed, "0,STATUS") !== "RDS_OK") {
      throw new CFRDSResponseError("Invalid debugger server info status", CFRDS_STATUS.RESPONSE_ERROR);
    }
    const port = wddxGetNumber(parsed, "0,DEBUG_SERVER_PORT");
    if (port === null || port < 0 || port > 0xffff) {
      throw new CFRDSResponseError("Invalid debugger server port", CFRDS_STATUS.RESPONSE_ERROR);
    }
    return Math.floor(port);
  }

  async debuggerBreakpointOnException(sessionName: string, enable: boolean): Promise<void> {
    if (sessionName === null || sessionName === undefined) {
      throw new CFRDSValidationError("sessionName is required", CFRDS_STATUS.PARAM_IS_NULL);
    }
    if (enable === null || enable === undefined) {
      throw new CFRDSValidationError("enable is required", CFRDS_STATUS.PARAM_IS_NULL);
    }
    const val = enable ? "true" : "false";
    const wddx = `<wddxPacket version='1.0'><header/><data><array length='1'><struct type='java.util.HashMap'><var name='COMMAND'><string>SESSION_BREAK_ON_EXCEPTION</string></var><var name='BREAK_ON_EXCEPTION'><boolean value='${val}'/></var></struct></array></data></wddxPacket>`;
    await this.send("DBGREQUEST", ["DBG_REQUEST", sessionName, wddx]);
  }

  async debuggerGlobalBreakpointOnException(sessionName: string, enable: boolean): Promise<void> {
    if (sessionName === null || sessionName === undefined) {
      throw new CFRDSValidationError("sessionName is required", CFRDS_STATUS.PARAM_IS_NULL);
    }
    if (enable === null || enable === undefined) {
      throw new CFRDSValidationError("enable is required", CFRDS_STATUS.PARAM_IS_NULL);
    }
    const val = enable ? "true" : "false";
    const wddx = `<wddxPacket version='1.0'><header/><data><array length='1'><struct type='java.util.HashMap'><var name='COMMAND'><string>GLOBAL_BREAK_ON_EXCEPTION</string></var><var name='BREAK_ON_EXCEPTION'><boolean value='${val}'/></var></struct></array></data></wddxPacket>`;
    await this.send("DBGREQUEST", ["DBG_REQUEST", sessionName, wddx]);
  }

  async debuggerBreakpoint(sessionName: string, filepath: string, line: number, enable: boolean): Promise<void> {
    if (sessionName === null || sessionName === undefined) {
      throw new CFRDSValidationError("sessionName is required", CFRDS_STATUS.PARAM_IS_NULL);
    }
    if (filepath === null || filepath === undefined) {
      throw new CFRDSValidationError("filepath is required", CFRDS_STATUS.PARAM_IS_NULL);
    }
    if (line === null || line === undefined) {
      throw new CFRDSValidationError("line is required", CFRDS_STATUS.PARAM_IS_NULL);
    }
    if (enable === null || enable === undefined) {
      throw new CFRDSValidationError("enable is required", CFRDS_STATUS.PARAM_IS_NULL);
    }
    const cmd = enable ? "SET_BREAKPOINT" : "UNSET_BREAKPOINT";
    const wddx = `<wddxPacket version='1.0'><header/><data><array length='1'><struct type='java.util.HashMap'><var name='COMMAND'><string>${cmd}</string></var><var name='FILE'><string>${escapeXml(filepath)}</string></var><var name='Y'><number>${line}</number></var><var name='SEQ'><number>1.0</number></var></struct></array></data></wddxPacket>`;
    await this.send("DBGREQUEST", ["DBG_REQUEST", sessionName, wddx]);
  }

  async debuggerClearAllBreakpoints(sessionName: string): Promise<void> {
    if (sessionName === null || sessionName === undefined) {
      throw new CFRDSValidationError("sessionName is required", CFRDS_STATUS.PARAM_IS_NULL);
    }
    const wddx = "<wddxPacket version='1.0'><header/><data><array length='1'><struct type='java.util.HashMap'><var name='COMMAND'><string>UNSET_ALL_BREAKPOINTS</string></var></struct></array></data></wddxPacket>";
    await this.send("DBGREQUEST", ["DBG_REQUEST", sessionName, wddx]);
  }

  /**
   * Fetches debugger events for the specified session.
   * NOTE: This is a long-polling request on the ColdFusion server that blocks until a debugger event occurs or times out.
   */
  private parseDebuggerEvent(raw: Buffer | null): DebuggerEvent | null {
    if (!raw || raw.length === 0) {
      return null;
    }
    const [count, offset] = parseNumber(raw, 0);
    if (count !== 1) {
      return null;
    }
    const [wddxXml] = parseString(raw, offset);
    if (!wddxXml) {
      return null;
    }
    const parsed = wddxDeserialize(wddxXml);
    if (!parsed) {
      return null;
    }

    // C accesses every field through the "0,<FIELD>" path, i.e. the first
    // element of the top-level WDDX array.
    const first = wddxGet(parsed, "0");
    const data: Record<string, any> =
      first && typeof first === "object" && !Array.isArray(first)
        ? first
        : (parsed && typeof parsed === "object" && !Array.isArray(parsed) ? parsed : {});

    const evtName =
      typeof data.EVENT === "string"
        ? data.EVENT
        : typeof data.COMMAND === "string"
          ? data.COMMAND
          : null;
    const threadName = data.THREAD || data.THREAD_ID || data.THREAD_NAME || "main";
    data.thread_name = threadName;
    data.thread_id = threadName;

    if (evtName === "CF_BREAKPOINT_SET") {
      return {
        type: CFRDS_DEBUGGER_EVENT_TYPE.BREAKPOINT_SET,
        data: {
          ...data,
          // C: cfrds_debugger_event_breakpoint_set_get_pathname/req_line/act_line
          pathname: data.CFML_PATH || "",
          req_line: Math.floor(data.REQ_LINE_NUM || 0),
          act_line: Math.floor(data.ACTUAL_LINE_NUM || 0),
          thread_name: threadName,
          thread_id: threadName,
        },
      };
    } else if (evtName === "BREAKPOINT" || evtName === "CF_BREAKPOINT_HIT") {
      return {
        type: CFRDS_DEBUGGER_EVENT_TYPE.BREAKPOINT,
        data: {
          ...data,
          // C: cfrds_debugger_event_breakpoint_get_source/line
          source: data.SOURCE || "",
          line: Math.floor(data.LINE || 0),
          thread_name: threadName,
          thread_id: threadName,
        },
      };
    } else if (evtName === "STEP" || evtName === "CF_STEP") {
      return {
        type: CFRDS_DEBUGGER_EVENT_TYPE.STEP,
        data: {
          ...data,
          source: data.SOURCE || "",
          line: Math.floor(data.LINE || 0),
          thread_name: threadName,
          thread_id: threadName,
        },
      };
    }
    return { type: CFRDS_DEBUGGER_EVENT_TYPE.UNKNOWN, data };
  }

  /**
   * Fetches debugger events for the specified session.
   * NOTE: This is a long-polling request on the ColdFusion server that blocks until a debugger event occurs or times out.
   */
  async debuggerGetDebugEvents(sessionName: string): Promise<DebuggerEvent | null> {
    if (sessionName === null || sessionName === undefined) {
      throw new CFRDSValidationError("sessionName is required", CFRDS_STATUS.PARAM_IS_NULL);
    }
    const raw = await this.send("DBGREQUEST", ["DBG_EVENTS", sessionName]);
    return this.parseDebuggerEvent(raw);
  }

  /**
   * Configures fetch flags and waits for debugger events.
   * NOTE: This is a long-polling request on the ColdFusion server that blocks until a debugger event occurs or times out.
   */
  async debuggerAllFetchFlagsEnabled(
    sessionName: string,
    threads: boolean,
    watch: boolean,
    scopes: boolean,
    cfTrace: boolean,
    javaTrace: boolean
  ): Promise<DebuggerEvent | null> {
    if (sessionName === null || sessionName === undefined) {
      throw new CFRDSValidationError("sessionName is required", CFRDS_STATUS.PARAM_IS_NULL);
    }
    if (threads === null || threads === undefined) {
      throw new CFRDSValidationError("threads is required", CFRDS_STATUS.PARAM_IS_NULL);
    }
    if (watch === null || watch === undefined) {
      throw new CFRDSValidationError("watch is required", CFRDS_STATUS.PARAM_IS_NULL);
    }
    if (scopes === null || scopes === undefined) {
      throw new CFRDSValidationError("scopes is required", CFRDS_STATUS.PARAM_IS_NULL);
    }
    if (cfTrace === null || cfTrace === undefined) {
      throw new CFRDSValidationError("cfTrace is required", CFRDS_STATUS.PARAM_IS_NULL);
    }
    if (javaTrace === null || javaTrace === undefined) {
      throw new CFRDSValidationError("javaTrace is required", CFRDS_STATUS.PARAM_IS_NULL);
    }
    const b = (v: boolean): string => (v ? "true" : "false");
    const wddx = `<wddxPacket version='1.0'><header/><data><struct type='java.util.HashMap'>` +
      `<var name='THREADS'><boolean value='${b(threads)}'/></var>` +
      `<var name='WATCH'><boolean value='${b(watch)}'/></var>` +
      `<var name='SCOPES'><boolean value='${b(scopes)}'/></var>` +
      `<var name='CF_TRACE'><boolean value='${b(cfTrace)}'/></var>` +
      `<var name='JAVA_TRACE'><boolean value='${b(javaTrace)}'/></var>` +
      `</struct></data></wddxPacket>`;
    const raw = await this.send("DBGREQUEST", ["DBG_EVENTS", sessionName, wddx]);
    return this.parseDebuggerEvent(raw);
  }

  async debuggerStepIn(sessionName: string, threadName: string): Promise<void> {
    if (sessionName === null || sessionName === undefined) {
      throw new CFRDSValidationError("sessionName is required", CFRDS_STATUS.PARAM_IS_NULL);
    }
    if (threadName === null || threadName === undefined) {
      throw new CFRDSValidationError("threadName is required", CFRDS_STATUS.PARAM_IS_NULL);
    }
    const wddx = `<wddxPacket version='1.0'><header/><data><array length='1'><struct type='java.util.HashMap'><var name='COMMAND'><string>STEP_IN</string></var><var name='THREAD'><string>${escapeXml(threadName)}</string></var></struct></array></data></wddxPacket>`;
    await this.send("DBGREQUEST", ["DBG_REQUEST", sessionName, wddx]);
  }

  async debuggerStepOver(sessionName: string, threadName: string): Promise<void> {
    if (sessionName === null || sessionName === undefined) {
      throw new CFRDSValidationError("sessionName is required", CFRDS_STATUS.PARAM_IS_NULL);
    }
    if (threadName === null || threadName === undefined) {
      throw new CFRDSValidationError("threadName is required", CFRDS_STATUS.PARAM_IS_NULL);
    }
    const wddx = `<wddxPacket version='1.0'><header/><data><array length='1'><struct type='java.util.HashMap'><var name='COMMAND'><string>STEP_OVER</string></var><var name='THREAD'><string>${escapeXml(threadName)}</string></var></struct></array></data></wddxPacket>`;
    await this.send("DBGREQUEST", ["DBG_REQUEST", sessionName, wddx]);
  }

  async debuggerStepOut(sessionName: string, threadName: string): Promise<void> {
    if (sessionName === null || sessionName === undefined) {
      throw new CFRDSValidationError("sessionName is required", CFRDS_STATUS.PARAM_IS_NULL);
    }
    if (threadName === null || threadName === undefined) {
      throw new CFRDSValidationError("threadName is required", CFRDS_STATUS.PARAM_IS_NULL);
    }
    const wddx = `<wddxPacket version='1.0'><header/><data><array length='1'><struct type='java.util.HashMap'><var name='COMMAND'><string>STEP_OUT</string></var><var name='THREAD'><string>${escapeXml(threadName)}</string></var></struct></array></data></wddxPacket>`;
    await this.send("DBGREQUEST", ["DBG_REQUEST", sessionName, wddx]);
  }

  async debuggerSyncStepIn(sessionName: string, threadName: string): Promise<DebuggerEvent | null> {
    if (sessionName === null || sessionName === undefined) {
      throw new CFRDSValidationError("sessionName is required", CFRDS_STATUS.PARAM_IS_NULL);
    }
    if (threadName === null || threadName === undefined) {
      throw new CFRDSValidationError("threadName is required", CFRDS_STATUS.PARAM_IS_NULL);
    }
    const wddx = `<wddxPacket version='1.0'><header/><data><array length='1'><struct type='java.util.HashMap'><var name='COMMAND'><string>SYNC_STEP_IN</string></var><var name='THREAD'><string>${escapeXml(threadName)}</string></var></struct></array></data></wddxPacket>`;
    const raw = await this.send("DBGREQUEST", ["DBG_REQUEST", sessionName, wddx]);
    return this.parseDebuggerEvent(raw);
  }

  async debuggerSyncStepOver(sessionName: string, threadName: string): Promise<DebuggerEvent | null> {
    if (sessionName === null || sessionName === undefined) {
      throw new CFRDSValidationError("sessionName is required", CFRDS_STATUS.PARAM_IS_NULL);
    }
    if (threadName === null || threadName === undefined) {
      throw new CFRDSValidationError("threadName is required", CFRDS_STATUS.PARAM_IS_NULL);
    }
    const wddx = `<wddxPacket version='1.0'><header/><data><array length='1'><struct type='java.util.HashMap'><var name='COMMAND'><string>SYNC_STEP_OVER</string></var><var name='THREAD'><string>${escapeXml(threadName)}</string></var></struct></array></data></wddxPacket>`;
    const raw = await this.send("DBGREQUEST", ["DBG_REQUEST", sessionName, wddx]);
    return this.parseDebuggerEvent(raw);
  }

  async debuggerSyncStepOut(sessionName: string, threadName: string): Promise<DebuggerEvent | null> {
    if (sessionName === null || sessionName === undefined) {
      throw new CFRDSValidationError("sessionName is required", CFRDS_STATUS.PARAM_IS_NULL);
    }
    if (threadName === null || threadName === undefined) {
      throw new CFRDSValidationError("threadName is required", CFRDS_STATUS.PARAM_IS_NULL);
    }
    const wddx = `<wddxPacket version='1.0'><header/><data><array length='1'><struct type='java.util.HashMap'><var name='COMMAND'><string>SYNC_STEP_OUT</string></var><var name='THREAD'><string>${escapeXml(threadName)}</string></var></struct></array></data></wddxPacket>`;
    const raw = await this.send("DBGREQUEST", ["DBG_REQUEST", sessionName, wddx]);
    return this.parseDebuggerEvent(raw);
  }

  async debuggerContinue(sessionName: string, threadName: string): Promise<void> {
    if (sessionName === null || sessionName === undefined) {
      throw new CFRDSValidationError("sessionName is required", CFRDS_STATUS.PARAM_IS_NULL);
    }
    if (threadName === null || threadName === undefined) {
      throw new CFRDSValidationError("threadName is required", CFRDS_STATUS.PARAM_IS_NULL);
    }
    const wddx = `<wddxPacket version='1.0'><header/><data><array length='1'><struct type='java.util.HashMap'><var name='COMMAND'><string>CONTINUE</string></var><var name='THREAD'><string>${escapeXml(threadName)}</string></var></struct></array></data></wddxPacket>`;
    await this.send("DBGREQUEST", ["DBG_REQUEST", sessionName, wddx]);
  }

  async debuggerGetCfVariables(sessionName: string, threadName: string): Promise<DebuggerEvent | null> {
    if (sessionName === null || sessionName === undefined) {
      throw new CFRDSValidationError("sessionName is required", CFRDS_STATUS.PARAM_IS_NULL);
    }
    if (threadName === null || threadName === undefined) {
      throw new CFRDSValidationError("threadName is required", CFRDS_STATUS.PARAM_IS_NULL);
    }
    const wddx = `<wddxPacket version='1.0'><header/><data><array length='1'><struct type='java.util.HashMap'><var name='COMMAND'><string>GET_CF_VARIABLES</string></var><var name='THREAD'><string>${escapeXml(threadName)}</string></var></struct></array></data></wddxPacket>`;
    const raw = await this.send("DBGREQUEST", ["DBG_REQUEST", sessionName, wddx]);
    return this.parseDebuggerEvent(raw);
  }

  async debuggerWatchExpression(sessionName: string, threadName: string, expression: string): Promise<void> {
    if (sessionName === null || sessionName === undefined) {
      throw new CFRDSValidationError("sessionName is required", CFRDS_STATUS.PARAM_IS_NULL);
    }
    if (threadName === null || threadName === undefined) {
      throw new CFRDSValidationError("threadName is required", CFRDS_STATUS.PARAM_IS_NULL);
    }
    if (expression === null || expression === undefined) {
      throw new CFRDSValidationError("expression is required", CFRDS_STATUS.PARAM_IS_NULL);
    }
    const wddx = `<wddxPacket version='1.0'><header/><data><array length='1'><struct type='java.util.HashMap'><var name='COMMAND'><string>GET_SINGLE_CF_VARIABLE</string></var><var name='VARIABLE_NAME'><string>${escapeXml(expression)}</string></var><var name='THREAD'><string>${escapeXml(threadName)}</string></var></struct></array></data></wddxPacket>`;
    await this.send("DBGREQUEST", ["DBG_REQUEST", sessionName, wddx]);
  }

  async debuggerSetVariable(sessionName: string, threadName: string, variable: string, value: string): Promise<void> {
    if (sessionName === null || sessionName === undefined) {
      throw new CFRDSValidationError("sessionName is required", CFRDS_STATUS.PARAM_IS_NULL);
    }
    if (threadName === null || threadName === undefined) {
      throw new CFRDSValidationError("threadName is required", CFRDS_STATUS.PARAM_IS_NULL);
    }
    if (variable === null || variable === undefined) {
      throw new CFRDSValidationError("variable is required", CFRDS_STATUS.PARAM_IS_NULL);
    }
    if (value === null || value === undefined) {
      throw new CFRDSValidationError("value is required", CFRDS_STATUS.PARAM_IS_NULL);
    }
    const wddx = `<wddxPacket version='1.0'><header/><data><array length='1'><struct type='java.util.HashMap'><var name='COMMAND'><string>SET_VARIABLE_VALUE</string></var><var name='VARIABLE_NAME'><string>${escapeXml(variable)}</string></var><var name='VARIABLE_VALUE'><string>${escapeXml(value)}</string></var><var name='THREAD'><string>${escapeXml(threadName)}</string></var></struct></array></data></wddxPacket>`;
    await this.send("DBGREQUEST", ["DBG_REQUEST", sessionName, wddx]);
  }

  async debuggerWatchVariables(sessionName: string, variables: string): Promise<void> {
    if (sessionName === null || sessionName === undefined) {
      throw new CFRDSValidationError("sessionName is required", CFRDS_STATUS.PARAM_IS_NULL);
    }
    if (variables === null || variables === undefined) {
      throw new CFRDSValidationError("variables is required", CFRDS_STATUS.PARAM_IS_NULL);
    }
    const vars = variables.split(",").map((v) => v.trim()).filter((v) => v.length > 0);
    const varTags = vars.map((v) => `<string>${escapeXml(v)}</string>`).join("");
    const wddx = `<wddxPacket version='1.0'><header/><data><array length='1'><struct type='java.util.HashMap'><var name='COMMAND'><string>SET_WATCH_VARIABLES</string></var><var name='WATCH'><array length='${vars.length}'>${varTags}</array></var></struct></array></data></wddxPacket>`;
    await this.send("DBGREQUEST", ["DBG_REQUEST", sessionName, wddx]);
  }

  async debuggerGetOutput(sessionName: string, threadName: string): Promise<string> {
    if (sessionName === null || sessionName === undefined) {
      throw new CFRDSValidationError("sessionName is required", CFRDS_STATUS.PARAM_IS_NULL);
    }
    if (threadName === null || threadName === undefined) {
      throw new CFRDSValidationError("threadName is required", CFRDS_STATUS.PARAM_IS_NULL);
    }
    const wddx = `<wddxPacket version='1.0'><header/><data><array length='1'><struct type='java.util.HashMap'><var name='COMMAND'><string>GET_OUTPUT</string></var><var name='BODY_ONLY'><boolean value='true'/></var><var name='THREAD'><string>${escapeXml(threadName)}</string></var></struct></array></data></wddxPacket>`;
    const raw = await this.send("DBGREQUEST", ["DBG_REQUEST", sessionName, wddx]);
    if (!raw || raw.length === 0) {
      return "";
    }
    const [count, offset] = parseNumber(raw, 0);
    if (count !== 1) {
      throw new CFRDSResponseError("Invalid debuggerGetOutput response count", CFRDS_STATUS.RESPONSE_ERROR);
    }
    const [wddxXml] = parseString(raw, offset);
    if (!wddxXml) {
      return "";
    }
    const parsed = wddxDeserialize(wddxXml);
    // C reads "0,VALUE" (the first element of the top-level WDDX array).
    const value = wddxGet(parsed, "0,VALUE");
    return value === null || value === undefined ? "" : String(value);
  }

  async debuggerSetScopeFilter(sessionName: string, filterStr: string): Promise<void> {
    if (sessionName === null || sessionName === undefined) {
      throw new CFRDSValidationError("sessionName is required", CFRDS_STATUS.PARAM_IS_NULL);
    }
    if (filterStr === null || filterStr === undefined) {
      throw new CFRDSValidationError("filterStr is required", CFRDS_STATUS.PARAM_IS_NULL);
    }
    const wddx = `<wddxPacket version='1.0'><header/><data><array length='1'><struct type='java.util.HashMap'><var name='COMMAND'><string>SET_SCOPE_FILTER</string></var><var name='FILTER'><string>${escapeXml(filterStr)}</string></var></struct></array></data></wddxPacket>`;
    await this.send("DBGREQUEST", ["DBG_REQUEST", sessionName, wddx]);
  }

  // Security Analyzer Operations

  /**
   * Parses the single JSON string returned by SECURITYANALYZER commands,
   * mirroring the C `parse_sa_json_response`: exactly one row, one JSON string
   * and (optionally) no trailing bytes, and a `status == "success"` field.
   */
  private parseSecurityAnalyzerJson(
    raw: Buffer,
    options: { requireSuccess?: boolean; requireEnd?: boolean } = {}
  ): Record<string, unknown> {
    const { requireSuccess = true, requireEnd = true } = options;

    const [count, offset] = parseNumber(raw, 0);
    if (count !== 1) {
      throw new CFRDSResponseError("Invalid security analyzer response count", CFRDS_STATUS.RESPONSE_ERROR);
    }

    const [jsonStr, endOffset] = parseString(raw, offset);
    if (requireEnd && endOffset !== raw.length) {
      throw new CFRDSResponseError(
        "Unexpected trailing bytes in security analyzer response",
        CFRDS_STATUS.RESPONSE_ERROR
      );
    }

    let parsed: unknown;
    try {
      parsed = JSON.parse(jsonStr);
    } catch {
      throw new CFRDSResponseError("Invalid JSON in security analyzer response", CFRDS_STATUS.RESPONSE_ERROR);
    }

    if (!parsed || typeof parsed !== "object" || Array.isArray(parsed)) {
      throw new CFRDSResponseError("Invalid security analyzer JSON object", CFRDS_STATUS.RESPONSE_ERROR);
    }

    const obj = parsed as Record<string, unknown>;
    if (requireSuccess && obj.status !== "success") {
      const message =
        typeof obj.errormessage === "string" ? (obj.errormessage as string) : "security analyzer request failed";
      throw new CFRDSResponseError(message, CFRDS_STATUS.RESPONSE_ERROR);
    }

    return obj;
  }

  async securityAnalyzerScan(pathnames: string, recursively: boolean = true, cores: number = 1): Promise<number> {
    if (pathnames === null || pathnames === undefined) {
      throw new CFRDSValidationError("pathnames is required", CFRDS_STATUS.PARAM_IS_NULL);
    }
    const raw = await this.send("SECURITYANALYZER", [
      "scan", pathnames, recursively ? "true" : "false", String(cores),
    ]);
    const parsed = this.parseSecurityAnalyzerJson(raw);
    const id = parsed.id;
    if (typeof id !== "number" || !Number.isInteger(id)) {
      throw new CFRDSResponseError("invalid or missing security analyzer id", CFRDS_STATUS.RESPONSE_ERROR);
    }
    return id;
  }

  async securityAnalyzerCancel(commandId: number): Promise<void> {
    if (commandId === null || commandId === undefined) {
      throw new CFRDSValidationError("commandId is required", CFRDS_STATUS.PARAM_IS_NULL);
    }
    const raw = await this.send("SECURITYANALYZER", ["cancel", String(commandId)]);
    this.parseSecurityAnalyzerJson(raw);
  }

  async securityAnalyzerStatus(commandId: number): Promise<SecurityAnalyzerStatus> {
    if (commandId === null || commandId === undefined) {
      throw new CFRDSValidationError("commandId is required", CFRDS_STATUS.PARAM_IS_NULL);
    }
    const raw = await this.send("SECURITYANALYZER", ["status", String(commandId)]);
    const parsed = this.parseSecurityAnalyzerJson(raw);

    const requireInt = (key: string): number => {
      const value = parsed[key];
      if (typeof value !== "number" || !Number.isInteger(value)) {
        throw new CFRDSResponseError(`invalid ${key}`, CFRDS_STATUS.RESPONSE_ERROR);
      }
      return value;
    };

    return {
      totalfiles: requireInt("totalfiles"),
      filesvisitedcount: requireInt("filesvisitedcount"),
      percentage: requireInt("percentage"),
      lastupdated: requireInt("lastupdated"),
    };
  }

  async securityAnalyzerResult(commandId: number): Promise<SecurityAnalyzerResult> {
    if (commandId === null || commandId === undefined) {
      throw new CFRDSValidationError("commandId is required", CFRDS_STATUS.PARAM_IS_NULL);
    }
    const raw = await this.send("SECURITYANALYZER", ["result", String(commandId)]);
    // The C implementation parses the report JSON directly without checking
    // `status` or trailing bytes.
    return this.parseSecurityAnalyzerJson(raw, { requireSuccess: false, requireEnd: false });
  }

  async securityAnalyzerClean(commandId: number): Promise<void> {
    if (commandId === null || commandId === undefined) {
      throw new CFRDSValidationError("commandId is required", CFRDS_STATUS.PARAM_IS_NULL);
    }
    const raw = await this.send("SECURITYANALYZER", ["clean", String(commandId)]);
    // C only requires a single row containing a JSON string.
    const [count, offset] = parseNumber(raw, 0);
    if (count !== 1) {
      throw new CFRDSResponseError("Invalid security analyzer clean response count", CFRDS_STATUS.RESPONSE_ERROR);
    }
    const [jsonStr] = parseString(raw, offset);
    if (!jsonStr) {
      throw new CFRDSResponseError("Missing security analyzer clean response", CFRDS_STATUS.RESPONSE_ERROR);
    }
  }

  // IDE Default
  async ideDefault(version: number = 1): Promise<IdeDefaultResult> {
    if (version === null || version === undefined) {
      throw new CFRDSValidationError("version is required", CFRDS_STATUS.PARAM_IS_NULL);
    }
    const raw = await this.send("IDE_DEFAULT", ["", `${version},`]);
    const [count, offset] = parseNumber(raw, 0);
    if (count !== 5) {
      throw new CFRDSResponseError("Invalid ideDefault response count", CFRDS_STATUS.RESPONSE_ERROR);
    }
    const [n1, o1] = parseString(raw, offset);
    const [sVer, o2] = parseString(raw, o1);
    const [cVer, o3] = parseString(raw, o2);
    const [n2, o4] = parseString(raw, o3);
    const [n3, endOffset] = parseString(raw, o4);
    if (endOffset !== raw.length) {
      throw new CFRDSResponseError("Unexpected trailing bytes in ideDefault response", CFRDS_STATUS.RESPONSE_ERROR);
    }
    return {
      num1: safeInt(n1),
      server_version: sVer,
      client_version: cVer,
      num2: safeInt(n2),
      num3: safeInt(n3),
    };
  }

  // Admin API Operations
  async adminapiDebuggingGetlogproperty(logdirectory: string): Promise<string> {
    if (logdirectory === null || logdirectory === undefined) {
      throw new CFRDSValidationError("logdirectory is required", CFRDS_STATUS.PARAM_IS_NULL);
    }
    const raw = await this.send("ADMINAPI", ["cfide.adminapi.debugging", "getlogproperty", logdirectory]);
    const [count, offset] = parseNumber(raw, 0);
    if (count !== 1) {
      throw new CFRDSResponseError("Invalid getlogproperty response count", CFRDS_STATUS.RESPONSE_ERROR);
    }
    const [xml, endOffset] = parseString(raw, offset);
    if (endOffset !== raw.length) {
      throw new CFRDSResponseError("Unexpected trailing bytes in getlogproperty response", CFRDS_STATUS.RESPONSE_ERROR);
    }
    if (!xml) {
      return "";
    }
    // C deserializes the WDDX packet and requires the data node to be a string.
    const data = wddxDeserialize(xml);
    if (typeof data !== "string") {
      throw new CFRDSResponseError("Invalid getlogproperty WDDX data", CFRDS_STATUS.RESPONSE_ERROR);
    }
    return data;
  }

  async adminapiExtensionsGetcustomtagpaths(): Promise<string[]> {
    const raw = await this.send("ADMINAPI", ["cfide.adminapi.extensions", "getcustomtagpaths"]);
    const [, offset] = parseNumber(raw, 0);
    const [xml] = parseString(raw, offset);
    const paths: string[] = [];
    if (xml) {
      const parsed = wddxDeserialize(xml);
      if (Array.isArray(parsed)) {
        for (const item of parsed) {
          if (typeof item === "string") {
            paths.push(item);
          }
        }
      } else if (typeof parsed === "string") {
        paths.push(parsed);
      }
    }
    return paths;
  }

  async adminapiExtensionsSetmapping(name: string, path: string): Promise<void> {
    if (name === null || name === undefined) {
      throw new CFRDSValidationError("name is required", CFRDS_STATUS.PARAM_IS_NULL);
    }
    if (path === null || path === undefined) {
      throw new CFRDSValidationError("path is required", CFRDS_STATUS.PARAM_IS_NULL);
    }
    const argStr = `name:${escapeMappingValue(name)};path:${escapeMappingValue(path)}`;
    await this.send("ADMINAPI", ["cfide.adminapi.extensions", "setmappings", argStr]);
  }

  async adminapiExtensionsDeletemapping(mapping: string): Promise<void> {
    if (mapping === null || mapping === undefined) {
      throw new CFRDSValidationError("mapping is required", CFRDS_STATUS.PARAM_IS_NULL);
    }
    // NOTE: "deleltemappings" (with the extra 'l') is a required typo hardcoded in the Adobe ColdFusion RDS backend.
    await this.send("ADMINAPI", ["cfide.adminapi.extensions", "deleltemappings", mapping]);
  }

  async adminapiExtensionsGetmappings(): Promise<AdminApiMappings> {
    const raw = await this.send("ADMINAPI", ["cfide.adminapi.extensions", "getmappings"]);
    const [, offset] = parseNumber(raw, 0);
    const [xml] = parseString(raw, offset);
    const keys: string[] = [];
    const values: string[] = [];
    const mappings: Record<string, string> = {};
    if (xml) {
      const root = parseXml(xml);
      const findStruct = (node: XmlNode): XmlNode | null => {
        if (node.tag === "struct") return node;
        for (const child of node.children) {
          const res = findStruct(child);
          if (res) return res;
        }
        return null;
      };
      const structNode = findStruct(root);
      if (structNode) {
        for (const child of structNode.children) {
          if (child.tag === "var") {
            const name = child.attrs.name;
            if (name) {
              let val = "";
              if (child.children.length > 0) {
                const valNode = child.children[0];
                const parsedVal = parseWddxNode(valNode);
                val = parsedVal !== null && parsedVal !== undefined ? String(parsedVal) : "";
              }
              keys.push(name);
              values.push(val);
              mappings[name] = val;
            }
          }
        }
      }
    }
    return {
      mappings,
      keys,
      values,
    };
  }

  // Graphing Operations
  async graphing(chartAttributes: string, seriesData: string[]): Promise<Buffer> {
    if (chartAttributes === null || chartAttributes === undefined) {
      throw new CFRDSValidationError("chartAttributes is required", CFRDS_STATUS.PARAM_IS_NULL);
    }
    if (seriesData === null || seriesData === undefined) {
      throw new CFRDSValidationError("seriesData is required", CFRDS_STATUS.PARAM_IS_NULL);
    }
    const args: (string | Buffer)[] = ["GRAPH", chartAttributes, String(seriesData.length), ...seriesData];
    return this.send("GRAPHING", args);
  }
}

export function cfrds_debugger_event_get_type(evt: DebuggerEvent | null): CFRDS_DEBUGGER_EVENT_TYPE {
  return evt ? evt.type : CFRDS_DEBUGGER_EVENT_TYPE.UNKNOWN;
}

export function cfrds_debugger_event_breakpoint_get_source(evt: DebuggerEvent | null): string | null {
  return evt && evt.data && typeof evt.data.source === "string" ? evt.data.source : null;
}

export function cfrds_debugger_event_breakpoint_get_line(evt: DebuggerEvent | null): number {
  return evt && evt.data && typeof evt.data.line === "number" ? evt.data.line : 0;
}

export function cfrds_debugger_event_breakpoint_get_scopes(evt: DebuggerEvent | null): any {
  return evt && evt.data ? evt.data.SCOPES : null;
}

export function cfrds_debugger_event_breakpoint_get_thread_name(evt: DebuggerEvent | null): string | null {
  return evt && evt.data && typeof evt.data.thread_name === "string" ? evt.data.thread_name : null;
}

export function cfrds_debugger_event_breakpoint_set_get_pathname(evt: DebuggerEvent | null): string | null {
  return evt && evt.data && typeof evt.data.pathname === "string" ? evt.data.pathname : null;
}

export function cfrds_debugger_event_breakpoint_set_get_req_line(evt: DebuggerEvent | null): number {
  return evt && evt.data && typeof evt.data.req_line === "number" ? evt.data.req_line : 0;
}

export function cfrds_debugger_event_breakpoint_set_get_act_line(evt: DebuggerEvent | null): number {
  return evt && evt.data && typeof evt.data.act_line === "number" ? evt.data.act_line : 0;
}

export function cfrds_debugger_event_get_scopes_count(evt: DebuggerEvent | null): number {
  if (evt && evt.data && evt.data.SCOPES) {
    if (Array.isArray(evt.data.SCOPES)) {
      return evt.data.SCOPES.length;
    }
    if (typeof evt.data.SCOPES === "object") {
      return Object.keys(evt.data.SCOPES).length;
    }
  }
  return 0;
}

export function cfrds_debugger_event_get_scopes_item_name(evt: DebuggerEvent | null, ndx: number): string | null {
  if (evt && evt.data && evt.data.SCOPES) {
    if (Array.isArray(evt.data.SCOPES)) {
      const item = evt.data.SCOPES[ndx];
      if (typeof item === "string") return item;
      if (item && typeof item === "object") {
        const keys = Object.keys(item);
        return keys.length > 0 ? keys[0] : null;
      }
      return null;
    }
    if (typeof evt.data.SCOPES === "object") {
      const keys = Object.keys(evt.data.SCOPES);
      return ndx < keys.length ? keys[ndx] : null;
    }
  }
  return null;
}

export function cfrds_debugger_event_get_scopes_item_value(evt: DebuggerEvent | null, ndx: number): any {
  if (evt && evt.data && evt.data.SCOPES) {
    if (Array.isArray(evt.data.SCOPES)) {
      const item = evt.data.SCOPES[ndx];
      if (item && typeof item === "object") {
        const values = Object.values(item);
        return values.length > 0 ? values[0] : null;
      }
      return item ?? null;
    }
    if (typeof evt.data.SCOPES === "object") {
      const values = Object.values(evt.data.SCOPES);
      return ndx < values.length ? values[ndx] : null;
    }
  }
  return null;
}

export function cfrds_debugger_event_get_scopes_item(evt: DebuggerEvent | null, ndx: number): string | null {
  return cfrds_debugger_event_get_scopes_item_name(evt, ndx);
}

export function cfrds_debugger_event_get_threads_count(evt: DebuggerEvent | null): number {
  if (evt && evt.data && evt.data.THREADS) {
    if (Array.isArray(evt.data.THREADS)) {
      return evt.data.THREADS.length;
    }
    if (typeof evt.data.THREADS === "object") {
      return Object.keys(evt.data.THREADS).length;
    }
  }
  return 0;
}

export function cfrds_debugger_event_get_threads_item_name(evt: DebuggerEvent | null, ndx: number): string | null {
  if (evt && evt.data && evt.data.THREADS) {
    if (Array.isArray(evt.data.THREADS)) {
      const item = evt.data.THREADS[ndx];
      if (Array.isArray(item)) {
        return item[0] !== undefined ? String(item[0]) : null;
      }
      if (typeof item === "string") return item;
      if (item && typeof item === "object") {
        return (item as any).name ?? Object.keys(item)[0] ?? null;
      }
      return null;
    }
    if (typeof evt.data.THREADS === "object") {
      const keys = Object.keys(evt.data.THREADS);
      return ndx < keys.length ? keys[ndx] : null;
    }
  }
  return null;
}

export function cfrds_debugger_event_get_threads_item_state(evt: DebuggerEvent | null, ndx: number): string | null {
  if (evt && evt.data && evt.data.THREADS) {
    if (Array.isArray(evt.data.THREADS)) {
      const item = evt.data.THREADS[ndx];
      if (Array.isArray(item)) {
        return item[1] !== undefined ? String(item[1]) : null;
      }
      if (item && typeof item === "object") {
        return (item as any).state ?? Object.values(item)[0] ?? null;
      }
      return null;
    }
    if (typeof evt.data.THREADS === "object") {
      const values = Object.values(evt.data.THREADS);
      return ndx < values.length ? String(values[ndx]) : null;
    }
  }
  return null;
}

export function cfrds_debugger_event_get_threads_item(evt: DebuggerEvent | null, ndx: number): string | null {
  return cfrds_debugger_event_get_threads_item_name(evt, ndx);
}

export function cfrds_debugger_event_get_watch_count(evt: DebuggerEvent | null): number {
  if (evt && evt.data && evt.data.WATCH) {
    if (typeof evt.data.WATCH === "object" && !Array.isArray(evt.data.WATCH)) {
      return Object.keys(evt.data.WATCH).length;
    }
    if (Array.isArray(evt.data.WATCH)) {
      return evt.data.WATCH.length;
    }
  }
  return 0;
}

export function cfrds_debugger_event_get_watch_item(evt: DebuggerEvent | null, ndx: number): string | null {
  if (evt && evt.data && evt.data.WATCH) {
    if (typeof evt.data.WATCH === "object" && !Array.isArray(evt.data.WATCH)) {
      const keys = Object.keys(evt.data.WATCH);
      return ndx < keys.length ? keys[ndx] : null;
    }
    if (Array.isArray(evt.data.WATCH)) {
      const item = evt.data.WATCH[ndx];
      if (typeof item === "string") return item;
      if (item && typeof item === "object") {
        const keys = Object.keys(item);
        return keys.length > 0 ? keys[0] : null;
      }
      return null;
    }
  }
  return null;
}

export function cfrds_debugger_event_get_cf_trace_count(evt: DebuggerEvent | null): number {
  if (evt && evt.data && Array.isArray(evt.data.CF_TRACE)) {
    return evt.data.CF_TRACE.length;
  }
  return 0;
}

export function cfrds_debugger_event_get_cf_trace_item(evt: DebuggerEvent | null, ndx: number): string | null {
  if (evt && evt.data && Array.isArray(evt.data.CF_TRACE)) {
    const item = evt.data.CF_TRACE[ndx];
    return typeof item === "string" ? item : null;
  }
  return null;
}

export function cfrds_debugger_event_get_java_trace_count(evt: DebuggerEvent | null): number {
  if (evt && evt.data && Array.isArray(evt.data.JAVA_TRACE)) {
    return evt.data.JAVA_TRACE.length;
  }
  return 0;
}

export function cfrds_debugger_event_get_java_trace_item(evt: DebuggerEvent | null, ndx: number): string | null {
  if (evt && evt.data && Array.isArray(evt.data.JAVA_TRACE)) {
    const item = evt.data.JAVA_TRACE[ndx];
    return typeof item === "string" ? item : null;
  }
  return null;
}

/*
 * Typed Security Analyzer result accessors.
 *
 * These mirror the C `cfrds_security_analyzer_result_*` functions operating on
 * the raw JSON report object returned by securityAnalyzerResult(). Return
 * values follow the C semantics: missing/invalid fields yield -1 / null, and
 * cfrds_security_analyzer_result_status() yields "" when status is absent.
 */

function saResultObj(value: SecurityAnalyzerResult | null): Record<string, unknown> | null {
  return value && typeof value === "object" && !Array.isArray(value) ? value : null;
}

function saInt(value: SecurityAnalyzerResult | null, key: string): number {
  const obj = saResultObj(value);
  if (!obj) return -1;
  const field = obj[key];
  return typeof field === "number" && Number.isInteger(field) ? field : -1;
}

function saString(value: SecurityAnalyzerResult | null, key: string): string | null {
  const obj = saResultObj(value);
  if (!obj) return null;
  const field = obj[key];
  return typeof field === "string" ? field : null;
}

function saArrayLength(value: SecurityAnalyzerResult | null, key: string): number {
  const obj = saResultObj(value);
  if (!obj) return -1;
  const field = obj[key];
  return Array.isArray(field) ? field.length : -1;
}

function saArrayItem(
  value: SecurityAnalyzerResult | null,
  arrayKey: string,
  ndx: number
): Record<string, unknown> | null {
  const obj = saResultObj(value);
  if (!obj) return null;
  const field = obj[arrayKey];
  if (!Array.isArray(field)) return null;
  const item = field[ndx];
  return item && typeof item === "object" && !Array.isArray(item) ? (item as Record<string, unknown>) : null;
}

function saArrayItemString(value: SecurityAnalyzerResult | null, arrayKey: string, ndx: number, fieldKey: string): string | null {
  const item = saArrayItem(value, arrayKey, ndx);
  if (!item) return null;
  const field = item[fieldKey];
  return typeof field === "string" ? field : null;
}

function saArrayItemInt(value: SecurityAnalyzerResult | null, arrayKey: string, ndx: number, fieldKey: string): number {
  const item = saArrayItem(value, arrayKey, ndx);
  if (!item) return -1;
  const field = item[fieldKey];
  return typeof field === "number" && Number.isInteger(field) ? field : -1;
}

export function cfrds_security_analyzer_result_totalfiles(value: SecurityAnalyzerResult | null): number {
  return saInt(value, "totalfiles");
}

export function cfrds_security_analyzer_result_filesvisitedcount(value: SecurityAnalyzerResult | null): number {
  return saInt(value, "filesvisitedcount");
}

export function cfrds_security_analyzer_result_errorsdescription_count(value: SecurityAnalyzerResult | null): number {
  return saArrayLength(value, "errorsdescription");
}

export function cfrds_security_analyzer_result_filesscanned_count(value: SecurityAnalyzerResult | null): number {
  return saArrayLength(value, "filesscanned");
}

export function cfrds_security_analyzer_result_filesscanned_item_result(value: SecurityAnalyzerResult | null, ndx: number): string | null {
  return saArrayItemString(value, "filesscanned", ndx, "result");
}

export function cfrds_security_analyzer_result_filesscanned_item_filename(value: SecurityAnalyzerResult | null, ndx: number): string | null {
  return saArrayItemString(value, "filesscanned", ndx, "filename");
}

export function cfrds_security_analyzer_result_filesnotscanned_count(value: SecurityAnalyzerResult | null): number {
  return saArrayLength(value, "filesnotscanned");
}

export function cfrds_security_analyzer_result_filesnotscanned_item_reason(value: SecurityAnalyzerResult | null, ndx: number): string | null {
  return saArrayItemString(value, "filesnotscanned", ndx, "reason");
}

export function cfrds_security_analyzer_result_filesnotscanned_item_filename(value: SecurityAnalyzerResult | null, ndx: number): string | null {
  return saArrayItemString(value, "filesnotscanned", ndx, "filename");
}

export function cfrds_security_analyzer_result_executorservice(value: SecurityAnalyzerResult | null): string | null {
  return saString(value, "executorservice");
}

export function cfrds_security_analyzer_result_percentage(value: SecurityAnalyzerResult | null): number {
  return saInt(value, "percentage");
}

export function cfrds_security_analyzer_result_files_count(value: SecurityAnalyzerResult | null): number {
  return saArrayLength(value, "files");
}

export function cfrds_security_analyzer_result_files_value(value: SecurityAnalyzerResult | null, ndx: number): string | null {
  const obj = saResultObj(value);
  if (!obj) return null;
  const field = obj["files"];
  if (!Array.isArray(field)) return null;
  const item = field[ndx];
  return typeof item === "string" ? item : null;
}

export function cfrds_security_analyzer_result_lastupdated(value: SecurityAnalyzerResult | null): number {
  return saInt(value, "lastupdated");
}

export function cfrds_security_analyzer_result_filesvisited_count(value: SecurityAnalyzerResult | null): number {
  return saArrayLength(value, "filesvisited");
}

export function cfrds_security_analyzer_result_filesnotscannedcount(value: SecurityAnalyzerResult | null): number {
  return saInt(value, "filesnotscannedcount");
}

export function cfrds_security_analyzer_result_filesscannedcount(value: SecurityAnalyzerResult | null): number {
  return saInt(value, "filesscannedcount");
}

export function cfrds_security_analyzer_result_id(value: SecurityAnalyzerResult | null): number {
  return saInt(value, "id");
}

export function cfrds_security_analyzer_result_errors_count(value: SecurityAnalyzerResult | null): number {
  return saArrayLength(value, "errors");
}

export function cfrds_security_analyzer_result_errors_item_errormessage(value: SecurityAnalyzerResult | null, ndx: number): string | null {
  return saArrayItemString(value, "errors", ndx, "errormessage");
}

export function cfrds_security_analyzer_result_errors_item_endline(value: SecurityAnalyzerResult | null, ndx: number): number {
  return saArrayItemInt(value, "errors", ndx, "endline");
}

export function cfrds_security_analyzer_result_errors_item_path(value: SecurityAnalyzerResult | null, ndx: number): string | null {
  return saArrayItemString(value, "errors", ndx, "path");
}

export function cfrds_security_analyzer_result_errors_item_vulnerablecode(value: SecurityAnalyzerResult | null, ndx: number): string | null {
  return saArrayItemString(value, "errors", ndx, "vulnerablecode");
}

export function cfrds_security_analyzer_result_errors_item_filename(value: SecurityAnalyzerResult | null, ndx: number): string | null {
  return saArrayItemString(value, "errors", ndx, "filename");
}

export function cfrds_security_analyzer_result_errors_item_beginline(value: SecurityAnalyzerResult | null, ndx: number): number {
  return saArrayItemInt(value, "errors", ndx, "beginline");
}

export function cfrds_security_analyzer_result_errors_item_column(value: SecurityAnalyzerResult | null, ndx: number): number {
  return saArrayItemInt(value, "errors", ndx, "column");
}

export function cfrds_security_analyzer_result_errors_item_error(value: SecurityAnalyzerResult | null, ndx: number): string | null {
  return saArrayItemString(value, "errors", ndx, "Error");
}

export function cfrds_security_analyzer_result_errors_item_begincolumn(value: SecurityAnalyzerResult | null, ndx: number): number {
  return saArrayItemInt(value, "errors", ndx, "begincolumn");
}

export function cfrds_security_analyzer_result_errors_item_type(value: SecurityAnalyzerResult | null, ndx: number): string | null {
  return saArrayItemString(value, "errors", ndx, "type");
}

export function cfrds_security_analyzer_result_errors_item_endcolumn(value: SecurityAnalyzerResult | null, ndx: number): number {
  return saArrayItemInt(value, "errors", ndx, "endcolumn");
}

export function cfrds_security_analyzer_result_errors_item_referencetype(value: SecurityAnalyzerResult | null, ndx: number): string | null {
  return saArrayItemString(value, "errors", ndx, "referencetype");
}

export function cfrds_security_analyzer_result_status(value: SecurityAnalyzerResult | null): string {
  const status = saString(value, "status");
  return status === null ? "" : status;
}

export { Server as ServerType };
