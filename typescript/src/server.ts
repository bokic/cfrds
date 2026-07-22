import {
  CFRDS_STATUS,
  CFRDS_DEBUGGER_EVENT_TYPE,
  CFRDSError,
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
import { encodePassword, parseNumber, parseString, parseBytearray, parseStringListItem, wddxDeserialize, parseXml, parseWddxNode, XmlNode } from "./parser";
import { sendRdsCommand } from "./transport";

function escapeXml(str: string): string {
  return str
    .replace(/&/g, "&amp;")
    .replace(/</g, "&lt;")
    .replace(/>/g, "&gt;")
    .replace(/"/g, "&quot;")
    .replace(/'/g, "&apos;");
}

export class Server {
  private ctx: ServerContext;

  constructor(
    hostname: string = "127.0.0.1",
    port: number = 8500,
    username: string = "admin",
    password: string = ""
  ) {
    this.ctx = {
      config: { host: hostname, port, username, password },
      encodedPassword: encodePassword(password),
      errorCode: 1,
      error: null,
    };
  }

  getHost(): string { return this.ctx.config.host; }
  getPort(): number { return this.ctx.config.port; }
  getUsername(): string { return this.ctx.config.username; }
  getPassword(): string { return this.ctx.config.password; }
  getError(): string | null { return this.ctx.error; }
  getErrorCode(): number { return this.ctx.errorCode; }
  clearError(): void { this.ctx.error = null; this.ctx.errorCode = 1; }

  async close(): Promise<void> {}

  // Browse Directory
  async browseDir(path: string): Promise<BrowseDirItem[]> {
    if (path === null || path === undefined) {
      throw new CFRDSError(CFRDS_STATUS.PARAM_IS_NULL, "path is required");
    }
    const raw = await sendRdsCommand(this.ctx, "BROWSEDIR", [path, ""]);
    const [total, offset] = parseNumber(raw, 0);
    if (total < 0 || (total !== 0 && total % 5 !== 0)) {
      throw new CFRDSError(
        CFRDS_STATUS.RESPONSE_ERROR,
        "Invalid total items count in browseDir response"
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

      const kind = strKind === "D:" ? "D" : "F";
      const permsNum = strPerms ? parseInt(strPerms, 10) : 0;
      const size = strSize ? parseInt(strSize, 10) : 0;

      let modified = 0;
      if (strTs && strTs.includes(",")) {
        const parts = strTs.split(",");
        const num1 = parseInt(parts[0], 10);
        const num2 = parseInt(parts[1], 10);
        modified = Math.floor((num1 + (num2 * 0x100000000)) / 10000) - 11644473600000;
      }

      const permsStr = ["-", "-", "-", "-", "-"];
      if (kind === "D") permsStr[0] = "D";
      if (permsNum & 0x01) permsStr[1] = "R";
      if (permsNum & 0x02) permsStr[2] = "H";
      if (permsNum & 0x10) permsStr[3] = "A";
      if (permsNum & 0x80) permsStr[4] = "N";

      items.push({
        kind,
        name: filename,
        permissions: permsStr.join(""),
        size,
        modified,
      });
    }
    return items;
  }

  // File Operations
  async fileRead(filepath: string): Promise<FileContent> {
    if (filepath === null || filepath === undefined) {
      throw new CFRDSError(CFRDS_STATUS.PARAM_IS_NULL, "filepath is required");
    }
    const raw = await sendRdsCommand(this.ctx, "FILEIO", [filepath, "READ", ""]);
    const [, offset] = parseNumber(raw, 0);
    const [dataBytes, o1] = parseBytearray(raw, offset);
    const [modified, o2] = parseString(raw, o1);
    const [permission] = parseString(raw, o2);
    return { data: dataBytes, size: dataBytes.length, modified, permission };
  }

  async fileWrite(filepath: string, content: string | Buffer): Promise<void> {
    if (filepath === null || filepath === undefined) {
      throw new CFRDSError(CFRDS_STATUS.PARAM_IS_NULL, "filepath is required");
    }
    if (content === null || content === undefined) {
      throw new CFRDSError(CFRDS_STATUS.PARAM_IS_NULL, "content is required");
    }
    const data = typeof content === "string" ? Buffer.from(content, "utf-8") : content;
    await sendRdsCommand(this.ctx, "FILEIO", [filepath, "WRITE", "", data]);
  }

  async fileRename(filepathFrom: string, filepathTo: string): Promise<void> {
    if (filepathFrom === null || filepathFrom === undefined) {
      throw new CFRDSError(CFRDS_STATUS.PARAM_IS_NULL, "filepathFrom is required");
    }
    if (filepathTo === null || filepathTo === undefined) {
      throw new CFRDSError(CFRDS_STATUS.PARAM_IS_NULL, "filepathTo is required");
    }
    await sendRdsCommand(this.ctx, "FILEIO", [filepathFrom, "RENAME", "", filepathTo]);
  }

  async fileRemove(filepath: string): Promise<void> {
    if (filepath === null || filepath === undefined) {
      throw new CFRDSError(CFRDS_STATUS.PARAM_IS_NULL, "filepath is required");
    }
    await sendRdsCommand(this.ctx, "FILEIO", [filepath, "REMOVE", "", "F"]);
  }

  async dirRemove(dirpath: string): Promise<void> {
    if (dirpath === null || dirpath === undefined) {
      throw new CFRDSError(CFRDS_STATUS.PARAM_IS_NULL, "dirpath is required");
    }
    await sendRdsCommand(this.ctx, "FILEIO", [dirpath, "REMOVE", "", "D"]);
  }

  async fileExists(pathname: string): Promise<boolean> {
    if (pathname === null || pathname === undefined) {
      throw new CFRDSError(CFRDS_STATUS.PARAM_IS_NULL, "pathname is required");
    }
    try {
      await sendRdsCommand(this.ctx, "FILEIO", [pathname, "EXISTENCE", "", ""]);
      return true;
    } catch (e) {
      if (e instanceof CFRDSError && e.status === CFRDS_STATUS.COMMAND_FAILED) {
        return false;
      }
      throw e;
    }
  }

  async dirCreate(dirpath: string): Promise<void> {
    if (dirpath === null || dirpath === undefined) {
      throw new CFRDSError(CFRDS_STATUS.PARAM_IS_NULL, "dirpath is required");
    }
    await sendRdsCommand(this.ctx, "FILEIO", [dirpath, "CREATE", "", ""]);
  }

  async cfRootDir(): Promise<string> {
    const raw = await sendRdsCommand(this.ctx, "FILEIO", ["", "CF_DIRECTORY"]);
    const [, offset] = parseNumber(raw, 0);
    const [pathStr] = parseString(raw, offset);
    return pathStr;
  }

  // SQL Operations
  async sqlDsninfo(): Promise<string[]> {
    const raw = await sendRdsCommand(this.ctx, "DBFUNCS", ["", "DSNINFO"]);
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
      throw new CFRDSError(CFRDS_STATUS.PARAM_IS_NULL, "connectionName is required");
    }
    const raw = await sendRdsCommand(this.ctx, "DBFUNCS", [connectionName, "TABLEINFO"]);
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
      throw new CFRDSError(CFRDS_STATUS.PARAM_IS_NULL, "connectionName is required");
    }
    if (tableName === null || tableName === undefined) {
      throw new CFRDSError(CFRDS_STATUS.PARAM_IS_NULL, "tableName is required");
    }
    const raw = await sendRdsCommand(this.ctx, "DBFUNCS", [connectionName, "COLUMNINFO", tableName]);
    const [cnt, offset] = parseNumber(raw, 0);
    const cols: SqlColumnInfoItem[] = [];
    let off = offset;
    for (let i = 0; i < cnt; i++) {
      const [item, o] = parseString(raw, off);
      off = o;
      const fields = parseStringListItem(item);
      const safeInt = (s: string): number => (s && /^\d+$/.test(s)) ? parseInt(s, 10) : 0;
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
      throw new CFRDSError(CFRDS_STATUS.PARAM_IS_NULL, "connectionName is required");
    }
    if (tableName === null || tableName === undefined) {
      throw new CFRDSError(CFRDS_STATUS.PARAM_IS_NULL, "tableName is required");
    }
    const raw = await sendRdsCommand(this.ctx, "DBFUNCS", [connectionName, "PRIMARYKEYS", tableName]);
    const [cnt, offset] = parseNumber(raw, 0);
    const keys: SqlPrimaryKeyItem[] = [];
    let off = offset;
    for (let i = 0; i < cnt; i++) {
      const [item, o] = parseString(raw, off);
      off = o;
      const fields = parseStringListItem(item);
      const safeInt = (s: string): number => (s && /^\d+$/.test(s)) ? parseInt(s, 10) : 0;
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
      throw new CFRDSError(CFRDS_STATUS.PARAM_IS_NULL, "connectionName is required");
    }
    if (tableName === null || tableName === undefined) {
      throw new CFRDSError(CFRDS_STATUS.PARAM_IS_NULL, "tableName is required");
    }
    const raw = await sendRdsCommand(this.ctx, "DBFUNCS", [connectionName, "FOREIGNKEYS", tableName]);
    const [cnt, offset] = parseNumber(raw, 0);
    const keys: SqlForeignKeyItem[] = [];
    let off = offset;
    for (let i = 0; i < cnt; i++) {
      const [item, o] = parseString(raw, off);
      off = o;
      const fields = parseStringListItem(item);
      const safeInt = (s: string): number => (s && /^\d+$/.test(s)) ? parseInt(s, 10) : 0;
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
      throw new CFRDSError(CFRDS_STATUS.PARAM_IS_NULL, "connectionName is required");
    }
    if (tableName === null || tableName === undefined) {
      throw new CFRDSError(CFRDS_STATUS.PARAM_IS_NULL, "tableName is required");
    }
    const raw = await sendRdsCommand(this.ctx, "DBFUNCS", [connectionName, "IMPORTEDKEYS", tableName]);
    const [cnt, offset] = parseNumber(raw, 0);
    const keys: SqlForeignKeyItem[] = [];
    let off = offset;
    for (let i = 0; i < cnt; i++) {
      const [item, o] = parseString(raw, off);
      off = o;
      const fields = parseStringListItem(item);
      const safeInt = (s: string): number => (s && /^\d+$/.test(s)) ? parseInt(s, 10) : 0;
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
      throw new CFRDSError(CFRDS_STATUS.PARAM_IS_NULL, "connectionName is required");
    }
    if (tableName === null || tableName === undefined) {
      throw new CFRDSError(CFRDS_STATUS.PARAM_IS_NULL, "tableName is required");
    }
    const raw = await sendRdsCommand(this.ctx, "DBFUNCS", [connectionName, "EXPORTEDKEYS", tableName]);
    const [cnt, offset] = parseNumber(raw, 0);
    const keys: SqlForeignKeyItem[] = [];
    let off = offset;
    for (let i = 0; i < cnt; i++) {
      const [item, o] = parseString(raw, off);
      off = o;
      const fields = parseStringListItem(item);
      const safeInt = (s: string): number => (s && /^\d+$/.test(s)) ? parseInt(s, 10) : 0;
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
      throw new CFRDSError(CFRDS_STATUS.PARAM_IS_NULL, "connectionName is required");
    }
    if (sql === null || sql === undefined) {
      throw new CFRDSError(CFRDS_STATUS.PARAM_IS_NULL, "sql is required");
    }
    const raw = await sendRdsCommand(this.ctx, "DBFUNCS", [connectionName, "SQLSTMNT", sql]);
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
      throw new CFRDSError(CFRDS_STATUS.PARAM_IS_NULL, "connectionName is required");
    }
    if (sql === null || sql === undefined) {
      throw new CFRDSError(CFRDS_STATUS.PARAM_IS_NULL, "sql is required");
    }
    const raw = await sendRdsCommand(this.ctx, "DBFUNCS", [connectionName, "SQLMETADATA", sql]);
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
    const raw = await sendRdsCommand(this.ctx, "DBFUNCS", ["", "SUPPORTEDCOMMANDS"]);
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
      throw new CFRDSError(CFRDS_STATUS.PARAM_IS_NULL, "connectionName is required");
    }
    const raw = await sendRdsCommand(this.ctx, "DBFUNCS", [connectionName, "DBDESCRIPTION"]);
    const [, offset] = parseNumber(raw, 0);
    const [item] = parseString(raw, offset);
    const fields = parseStringListItem(item);
    return fields[0] || item;
  }

  // Debugger Operations
  async debuggerStart(): Promise<string> {
    const raw = await sendRdsCommand(this.ctx, "DBGREQUEST", ["DBG_START", ""]);
    const [, offset] = parseNumber(raw, 0);
    const [sessionId] = parseString(raw, offset);
    return sessionId;
  }

  async debuggerStop(sessionName: string): Promise<void> {
    if (sessionName === null || sessionName === undefined) {
      throw new CFRDSError(CFRDS_STATUS.PARAM_IS_NULL, "sessionName is required");
    }
    await sendRdsCommand(this.ctx, "DBGREQUEST", ["DBG_STOP", sessionName]);
  }

  async debuggerGetServerInfo(sessionName: string): Promise<number> {
    if (sessionName === null || sessionName === undefined) {
      throw new CFRDSError(CFRDS_STATUS.PARAM_IS_NULL, "sessionName is required");
    }
    const raw = await sendRdsCommand(this.ctx, "DBGREQUEST", ["DBG_GET_DEBUG_SERVER_INFO", sessionName]);
    const [, offset] = parseNumber(raw, 0);
    const [wddxXml] = parseString(raw, offset);
    const parsed = wddxDeserialize(wddxXml);
    return parsed && typeof parsed.DEBUG_SERVER_PORT === "number" ? Math.floor(parsed.DEBUG_SERVER_PORT) : 0;
  }

  async debuggerBreakpointOnException(sessionName: string, enable: boolean): Promise<void> {
    if (sessionName === null || sessionName === undefined) {
      throw new CFRDSError(CFRDS_STATUS.PARAM_IS_NULL, "sessionName is required");
    }
    if (enable === null || enable === undefined) {
      throw new CFRDSError(CFRDS_STATUS.PARAM_IS_NULL, "enable is required");
    }
    const val = enable ? "true" : "false";
    const wddx = `<wddxPacket version='1.0'><header/><data><array length='1'><struct type='java.util.HashMap'><var name='COMMAND'><string>SESSION_BREAK_ON_EXCEPTION</string></var><var name='BREAK_ON_EXCEPTION'><boolean value='${val}'/></var></struct></array></data></wddxPacket>`;
    await sendRdsCommand(this.ctx, "DBGREQUEST", ["DBG_REQUEST", sessionName, wddx]);
  }

  async debuggerBreakpoint(sessionName: string, filepath: string, line: number, enable: boolean): Promise<void> {
    if (sessionName === null || sessionName === undefined) {
      throw new CFRDSError(CFRDS_STATUS.PARAM_IS_NULL, "sessionName is required");
    }
    if (filepath === null || filepath === undefined) {
      throw new CFRDSError(CFRDS_STATUS.PARAM_IS_NULL, "filepath is required");
    }
    if (line === null || line === undefined) {
      throw new CFRDSError(CFRDS_STATUS.PARAM_IS_NULL, "line is required");
    }
    if (enable === null || enable === undefined) {
      throw new CFRDSError(CFRDS_STATUS.PARAM_IS_NULL, "enable is required");
    }
    const cmd = enable ? "SET_BREAKPOINT" : "UNSET_BREAKPOINT";
    const wddx = `<wddxPacket version='1.0'><header/><data><array length='1'><struct type='java.util.HashMap'><var name='COMMAND'><string>${cmd}</string></var><var name='FILE'><string>${escapeXml(filepath)}</string></var><var name='Y'><number>${line}</number></var><var name='SEQ'><number>1.0</number></var></struct></array></data></wddxPacket>`;
    await sendRdsCommand(this.ctx, "DBGREQUEST", ["DBG_REQUEST", sessionName, wddx]);
  }

  async debuggerClearAllBreakpoints(sessionName: string): Promise<void> {
    if (sessionName === null || sessionName === undefined) {
      throw new CFRDSError(CFRDS_STATUS.PARAM_IS_NULL, "sessionName is required");
    }
    const wddx = "<wddxPacket version='1.0'><header/><data><array length='1'><struct type='java.util.HashMap'><var name='COMMAND'><string>UNSET_ALL_BREAKPOINTS</string></var></struct></array></data></wddxPacket>";
    await sendRdsCommand(this.ctx, "DBGREQUEST", ["DBG_REQUEST", sessionName, wddx]);
  }

  /**
   * Fetches debugger events for the specified session.
   * NOTE: This is a long-polling request on the ColdFusion server that blocks until a debugger event occurs or times out.
   */
  private parseDebuggerEvent(raw: Buffer | null): DebuggerEvent | null {
    if (!raw || raw.length === 0) {
      return null;
    }
    const [, offset] = parseNumber(raw, 0);
    const [wddxXml] = parseString(raw, offset);
    if (!wddxXml) {
      return null;
    }
    const parsed = wddxDeserialize(wddxXml);
    if (!parsed) {
      return null;
    }
    const data = Array.isArray(parsed) ? (parsed[0] || {}) : parsed;

    const evtName = data.EVENT || data.COMMAND;
    const threadName = data.THREAD || data.THREAD_ID || data.THREAD_NAME || "main";
    data.thread_name = threadName;
    data.thread_id = threadName;

    if (evtName === "CF_BREAKPOINT_SET") {
      return {
        type: CFRDS_DEBUGGER_EVENT_TYPE.BREAKPOINT_SET,
        data: {
          pathname: data.CFML_PATH || data.FILE || "",
          req_line: Math.floor(data.REQ_LINE_NUM || 0),
          act_line: Math.floor(data.ACTUAL_LINE_NUM || 0),
          thread_name: threadName,
          thread_id: threadName,
          ...data,
        },
      };
    } else if (evtName === "CF_BREAKPOINT_HIT" || evtName === "CF_STEP" || evtName === "BREAKPOINT" || evtName === "STEP") {
      const type = String(evtName).includes("BREAKPOINT")
        ? CFRDS_DEBUGGER_EVENT_TYPE.BREAKPOINT
        : CFRDS_DEBUGGER_EVENT_TYPE.STEP;
      return {
        type,
        data: {
          source: data.CFML_PATH || data.FILE || "",
          line: Math.floor(data.REQ_LINE_NUM || data.LINE || 0),
          thread_name: threadName,
          thread_id: threadName,
          ...data,
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
      throw new CFRDSError(CFRDS_STATUS.PARAM_IS_NULL, "sessionName is required");
    }
    const raw = await sendRdsCommand(this.ctx, "DBGREQUEST", ["DBG_EVENTS", sessionName]);
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
      throw new CFRDSError(CFRDS_STATUS.PARAM_IS_NULL, "sessionName is required");
    }
    if (threads === null || threads === undefined) {
      throw new CFRDSError(CFRDS_STATUS.PARAM_IS_NULL, "threads is required");
    }
    if (watch === null || watch === undefined) {
      throw new CFRDSError(CFRDS_STATUS.PARAM_IS_NULL, "watch is required");
    }
    if (scopes === null || scopes === undefined) {
      throw new CFRDSError(CFRDS_STATUS.PARAM_IS_NULL, "scopes is required");
    }
    if (cfTrace === null || cfTrace === undefined) {
      throw new CFRDSError(CFRDS_STATUS.PARAM_IS_NULL, "cfTrace is required");
    }
    if (javaTrace === null || javaTrace === undefined) {
      throw new CFRDSError(CFRDS_STATUS.PARAM_IS_NULL, "javaTrace is required");
    }
    const b = (v: boolean): string => (v ? "true" : "false");
    const wddx = `<wddxPacket version='1.0'><header/><data><struct type='java.util.HashMap'>` +
      `<var name='THREADS'><boolean value='${b(threads)}'/></var>` +
      `<var name='WATCH'><boolean value='${b(watch)}'/></var>` +
      `<var name='SCOPES'><boolean value='${b(scopes)}'/></var>` +
      `<var name='CF_TRACE'><boolean value='${b(cfTrace)}'/></var>` +
      `<var name='JAVA_TRACE'><boolean value='${b(javaTrace)}'/></var>` +
      `</struct></data></wddxPacket>`;
    const raw = await sendRdsCommand(this.ctx, "DBGREQUEST", ["DBG_EVENTS", sessionName, wddx]);
    return this.parseDebuggerEvent(raw);
  }

  async debuggerStepIn(sessionName: string, threadName: string): Promise<void> {
    if (sessionName === null || sessionName === undefined) {
      throw new CFRDSError(CFRDS_STATUS.PARAM_IS_NULL, "sessionName is required");
    }
    if (threadName === null || threadName === undefined) {
      throw new CFRDSError(CFRDS_STATUS.PARAM_IS_NULL, "threadName is required");
    }
    const wddx = `<wddxPacket version='1.0'><header/><data><array length='1'><struct type='java.util.HashMap'><var name='COMMAND'><string>STEP_IN</string></var><var name='THREAD'><string>${escapeXml(threadName)}</string></var></struct></array></data></wddxPacket>`;
    await sendRdsCommand(this.ctx, "DBGREQUEST", ["DBG_REQUEST", sessionName, wddx]);
  }

  async debuggerStepOver(sessionName: string, threadName: string): Promise<void> {
    if (sessionName === null || sessionName === undefined) {
      throw new CFRDSError(CFRDS_STATUS.PARAM_IS_NULL, "sessionName is required");
    }
    if (threadName === null || threadName === undefined) {
      throw new CFRDSError(CFRDS_STATUS.PARAM_IS_NULL, "threadName is required");
    }
    const wddx = `<wddxPacket version='1.0'><header/><data><array length='1'><struct type='java.util.HashMap'><var name='COMMAND'><string>STEP_OVER</string></var><var name='THREAD'><string>${escapeXml(threadName)}</string></var></struct></array></data></wddxPacket>`;
    await sendRdsCommand(this.ctx, "DBGREQUEST", ["DBG_REQUEST", sessionName, wddx]);
  }

  async debuggerStepOut(sessionName: string, threadName: string): Promise<void> {
    if (sessionName === null || sessionName === undefined) {
      throw new CFRDSError(CFRDS_STATUS.PARAM_IS_NULL, "sessionName is required");
    }
    if (threadName === null || threadName === undefined) {
      throw new CFRDSError(CFRDS_STATUS.PARAM_IS_NULL, "threadName is required");
    }
    const wddx = `<wddxPacket version='1.0'><header/><data><array length='1'><struct type='java.util.HashMap'><var name='COMMAND'><string>STEP_OUT</string></var><var name='THREAD'><string>${escapeXml(threadName)}</string></var></struct></array></data></wddxPacket>`;
    await sendRdsCommand(this.ctx, "DBGREQUEST", ["DBG_REQUEST", sessionName, wddx]);
  }

  async debuggerContinue(sessionName: string, threadName: string): Promise<void> {
    if (sessionName === null || sessionName === undefined) {
      throw new CFRDSError(CFRDS_STATUS.PARAM_IS_NULL, "sessionName is required");
    }
    if (threadName === null || threadName === undefined) {
      throw new CFRDSError(CFRDS_STATUS.PARAM_IS_NULL, "threadName is required");
    }
    const wddx = `<wddxPacket version='1.0'><header/><data><array length='1'><struct type='java.util.HashMap'><var name='COMMAND'><string>CONTINUE</string></var><var name='THREAD'><string>${escapeXml(threadName)}</string></var></struct></array></data></wddxPacket>`;
    await sendRdsCommand(this.ctx, "DBGREQUEST", ["DBG_REQUEST", sessionName, wddx]);
  }

  async debuggerWatchExpression(sessionName: string, threadName: string, expression: string): Promise<void> {
    if (sessionName === null || sessionName === undefined) {
      throw new CFRDSError(CFRDS_STATUS.PARAM_IS_NULL, "sessionName is required");
    }
    if (threadName === null || threadName === undefined) {
      throw new CFRDSError(CFRDS_STATUS.PARAM_IS_NULL, "threadName is required");
    }
    if (expression === null || expression === undefined) {
      throw new CFRDSError(CFRDS_STATUS.PARAM_IS_NULL, "expression is required");
    }
    const wddx = `<wddxPacket version='1.0'><header/><data><array length='1'><struct type='java.util.HashMap'><var name='COMMAND'><string>GET_SINGLE_CF_VARIABLE</string></var><var name='VARIABLE_NAME'><string>${escapeXml(expression)}</string></var><var name='THREAD'><string>${escapeXml(threadName)}</string></var></struct></array></data></wddxPacket>`;
    await sendRdsCommand(this.ctx, "DBGREQUEST", ["DBG_REQUEST", sessionName, wddx]);
  }

  async debuggerSetVariable(sessionName: string, threadName: string, variable: string, value: string): Promise<void> {
    if (sessionName === null || sessionName === undefined) {
      throw new CFRDSError(CFRDS_STATUS.PARAM_IS_NULL, "sessionName is required");
    }
    if (threadName === null || threadName === undefined) {
      throw new CFRDSError(CFRDS_STATUS.PARAM_IS_NULL, "threadName is required");
    }
    if (variable === null || variable === undefined) {
      throw new CFRDSError(CFRDS_STATUS.PARAM_IS_NULL, "variable is required");
    }
    if (value === null || value === undefined) {
      throw new CFRDSError(CFRDS_STATUS.PARAM_IS_NULL, "value is required");
    }
    const wddx = `<wddxPacket version='1.0'><header/><data><array length='1'><struct type='java.util.HashMap'><var name='COMMAND'><string>SET_VARIABLE_VALUE</string></var><var name='VARIABLE_NAME'><string>${escapeXml(variable)}</string></var><var name='VARIABLE_VALUE'><string>${escapeXml(value)}</string></var><var name='THREAD'><string>${escapeXml(threadName)}</string></var></struct></array></data></wddxPacket>`;
    await sendRdsCommand(this.ctx, "DBGREQUEST", ["DBG_REQUEST", sessionName, wddx]);
  }

  async debuggerWatchVariables(sessionName: string, variables: string): Promise<void> {
    if (sessionName === null || sessionName === undefined) {
      throw new CFRDSError(CFRDS_STATUS.PARAM_IS_NULL, "sessionName is required");
    }
    if (variables === null || variables === undefined) {
      throw new CFRDSError(CFRDS_STATUS.PARAM_IS_NULL, "variables is required");
    }
    const vars = variables.split(",").map((v) => v.trim()).filter((v) => v.length > 0);
    const varTags = vars.map((v) => `<string>${escapeXml(v)}</string>`).join("");
    const wddx = `<wddxPacket version='1.0'><header/><data><array length='1'><struct type='java.util.HashMap'><var name='COMMAND'><string>SET_WATCH_VARIABLES</string></var><var name='WATCH'><array length='${vars.length}'>${varTags}</array></var></struct></array></data></wddxPacket>`;
    await sendRdsCommand(this.ctx, "DBGREQUEST", ["DBG_REQUEST", sessionName, wddx]);
  }

  async debuggerGetOutput(sessionName: string, threadName: string): Promise<string> {
    if (sessionName === null || sessionName === undefined) {
      throw new CFRDSError(CFRDS_STATUS.PARAM_IS_NULL, "sessionName is required");
    }
    if (threadName === null || threadName === undefined) {
      throw new CFRDSError(CFRDS_STATUS.PARAM_IS_NULL, "threadName is required");
    }
    const wddx = `<wddxPacket version='1.0'><header/><data><array length='1'><struct type='java.util.HashMap'><var name='COMMAND'><string>GET_OUTPUT</string></var><var name='BODY_ONLY'><boolean value='true'/></var><var name='THREAD'><string>${escapeXml(threadName)}</string></var></struct></array></data></wddxPacket>`;
    const raw = await sendRdsCommand(this.ctx, "DBGREQUEST", ["DBG_REQUEST", sessionName, wddx]);
    if (!raw || raw.length === 0) {
      return "";
    }
    const [, offset] = parseNumber(raw, 0);
    const [wddxXml] = parseString(raw, offset);
    if (!wddxXml) {
      return "";
    }
    const parsed = wddxDeserialize(wddxXml);
    if (parsed && parsed.VALUE !== undefined) {
      return String(parsed.VALUE);
    }
    return "";
  }

  async debuggerSetScopeFilter(sessionName: string, filterStr: string): Promise<void> {
    if (sessionName === null || sessionName === undefined) {
      throw new CFRDSError(CFRDS_STATUS.PARAM_IS_NULL, "sessionName is required");
    }
    if (filterStr === null || filterStr === undefined) {
      throw new CFRDSError(CFRDS_STATUS.PARAM_IS_NULL, "filterStr is required");
    }
    const wddx = `<wddxPacket version='1.0'><header/><data><array length='1'><struct type='java.util.HashMap'><var name='COMMAND'><string>SET_SCOPE_FILTER</string></var><var name='FILTER'><string>${escapeXml(filterStr)}</string></var></struct></array></data></wddxPacket>`;
    await sendRdsCommand(this.ctx, "DBGREQUEST", ["DBG_REQUEST", sessionName, wddx]);
  }

  // Security Analyzer Operations
  async securityAnalyzerScan(pathnames: string, recursively: boolean = true, cores: number = 1): Promise<number> {
    if (pathnames === null || pathnames === undefined) {
      throw new CFRDSError(CFRDS_STATUS.PARAM_IS_NULL, "pathnames is required");
    }
    const raw = await sendRdsCommand(this.ctx, "SECURITYANALYZER", [
      "scan", pathnames, recursively ? "true" : "false", String(cores),
    ]);
    const [, offset] = parseNumber(raw, 0);
    const [resStr] = parseString(raw, offset);
    try {
      const parsed = JSON.parse(resStr);
      return typeof parsed.id === "number" ? parsed.id : (parseInt(resStr, 10) || 0);
    } catch {
      return parseInt(resStr, 10) || 0;
    }
  }

  async securityAnalyzerCancel(commandId: number): Promise<void> {
    if (commandId === null || commandId === undefined) {
      throw new CFRDSError(CFRDS_STATUS.PARAM_IS_NULL, "commandId is required");
    }
    await sendRdsCommand(this.ctx, "SECURITYANALYZER", ["cancel", String(commandId)]);
  }

  async securityAnalyzerStatus(commandId: number): Promise<SecurityAnalyzerStatus> {
    if (commandId === null || commandId === undefined) {
      throw new CFRDSError(CFRDS_STATUS.PARAM_IS_NULL, "commandId is required");
    }
    const raw = await sendRdsCommand(this.ctx, "SECURITYANALYZER", ["status", String(commandId)]);
    const [, offset] = parseNumber(raw, 0);
    const [totStr, o1] = parseString(raw, offset);
    const [visStr, o2] = parseString(raw, o1);
    const [pctStr, o3] = parseString(raw, o2);
    const [updStr] = parseString(raw, o3);
    const safeInt = (s: string): number => (/^-?\d+$/.test(s)) ? parseInt(s, 10) : 0;
    return {
      totalfiles: safeInt(totStr),
      filesvisitedcount: safeInt(visStr),
      percentage: safeInt(pctStr),
      lastupdated: safeInt(updStr),
    };
  }

  async securityAnalyzerResult(commandId: number): Promise<Record<string, unknown> | null> {
    if (commandId === null || commandId === undefined) {
      throw new CFRDSError(CFRDS_STATUS.PARAM_IS_NULL, "commandId is required");
    }
    const raw = await sendRdsCommand(this.ctx, "SECURITYANALYZER", ["result", String(commandId)]);
    const [, offset] = parseNumber(raw, 0);
    const [resStr] = parseString(raw, offset);
    try {
      return JSON.parse(resStr);
    } catch {
      return { raw: resStr };
    }
  }

  async securityAnalyzerClean(commandId: number): Promise<void> {
    if (commandId === null || commandId === undefined) {
      throw new CFRDSError(CFRDS_STATUS.PARAM_IS_NULL, "commandId is required");
    }
    await sendRdsCommand(this.ctx, "SECURITYANALYZER", ["clean", String(commandId)]);
  }

  // IDE Default
  async ideDefault(version: number = 1): Promise<IdeDefaultResult> {
    if (version === null || version === undefined) {
      throw new CFRDSError(CFRDS_STATUS.PARAM_IS_NULL, "version is required");
    }
    const raw = await sendRdsCommand(this.ctx, "IDE_DEFAULT", ["", `${version},`]);
    const [, offset] = parseNumber(raw, 0);
    const [n1, o1] = parseString(raw, offset);
    const [sVer, o2] = parseString(raw, o1);
    const [cVer, o3] = parseString(raw, o2);
    const [n2, o4] = parseString(raw, o3);
    const [n3] = parseString(raw, o4);
    const safeInt = (s: string): number => (/^-?\d+$/.test(s)) ? parseInt(s, 10) : 0;
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
      throw new CFRDSError(CFRDS_STATUS.PARAM_IS_NULL, "logdirectory is required");
    }
    const raw = await sendRdsCommand(this.ctx, "ADMINAPI", ["cfide.adminapi.debugging", "getlogproperty", logdirectory]);
    const [, offset] = parseNumber(raw, 0);
    const [propStr] = parseString(raw, offset);
    return propStr;
  }

  async adminapiExtensionsGetcustomtagpaths(): Promise<string[]> {
    const raw = await sendRdsCommand(this.ctx, "ADMINAPI", ["cfide.adminapi.extensions", "getcustomtagpaths"]);
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
      throw new CFRDSError(CFRDS_STATUS.PARAM_IS_NULL, "name is required");
    }
    if (path === null || path === undefined) {
      throw new CFRDSError(CFRDS_STATUS.PARAM_IS_NULL, "path is required");
    }
    const wddx = `<wddxPacket version='1.0'><header/><data><struct><var name='${escapeXml(name)}'><string>${escapeXml(path)}</string></var></struct></data></wddxPacket>`;
    await sendRdsCommand(this.ctx, "ADMINAPI", ["cfide.adminapi.extensions", "setmappings", wddx]);
  }

  async adminapiExtensionsDeletemapping(mapping: string): Promise<void> {
    if (mapping === null || mapping === undefined) {
      throw new CFRDSError(CFRDS_STATUS.PARAM_IS_NULL, "mapping is required");
    }
    await sendRdsCommand(this.ctx, "ADMINAPI", ["cfide.adminapi.extensions", "deleltemappings", mapping]);
  }

  async adminapiExtensionsGetmappings(): Promise<AdminApiMappings> {
    const raw = await sendRdsCommand(this.ctx, "ADMINAPI", ["cfide.adminapi.extensions", "getmappings"]);
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
      throw new CFRDSError(CFRDS_STATUS.PARAM_IS_NULL, "chartAttributes is required");
    }
    if (seriesData === null || seriesData === undefined) {
      throw new CFRDSError(CFRDS_STATUS.PARAM_IS_NULL, "seriesData is required");
    }
    const args: (string | Buffer)[] = ["GRAPH", chartAttributes, String(seriesData.length), ...seriesData];
    return sendRdsCommand(this.ctx, "GRAPHING", args);
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
  if (evt && evt.data && Array.isArray(evt.data.SCOPES)) {
    return evt.data.SCOPES.length;
  }
  return 0;
}

export function cfrds_debugger_event_get_scopes_item(evt: DebuggerEvent | null, ndx: number): string | null {
  if (evt && evt.data && Array.isArray(evt.data.SCOPES)) {
    const item = evt.data.SCOPES[ndx];
    return typeof item === "string" ? item : null;
  }
  return null;
}

export function cfrds_debugger_event_get_threads_count(evt: DebuggerEvent | null): number {
  if (evt && evt.data && Array.isArray(evt.data.THREADS)) {
    return evt.data.THREADS.length;
  }
  return 0;
}

export function cfrds_debugger_event_get_threads_item(evt: DebuggerEvent | null, ndx: number): string | null {
  if (evt && evt.data && Array.isArray(evt.data.THREADS)) {
    const item = evt.data.THREADS[ndx];
    return typeof item === "string" ? item : null;
  }
  return null;
}

export function cfrds_debugger_event_get_watch_count(evt: DebuggerEvent | null): number {
  if (evt && evt.data && Array.isArray(evt.data.WATCH)) {
    return evt.data.WATCH.length;
  }
  return 0;
}

export function cfrds_debugger_event_get_watch_item(evt: DebuggerEvent | null, ndx: number): string | null {
  if (evt && evt.data && Array.isArray(evt.data.WATCH)) {
    const item = evt.data.WATCH[ndx];
    return typeof item === "string" ? item : null;
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

export { Server as ServerType };
