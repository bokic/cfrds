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
import { encodePassword, parseNumber, parseString, parseBytearray, parseStringListItem } from "./parser";
import { sendRdsCommand } from "./transport";

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
    const raw = await sendRdsCommand(this.ctx, "BROWSEDIR", [path, ""]);
    const [total, offset] = parseNumber(raw, 0);
    const cnt = Math.floor(total / 5);
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
        modified = Math.floor((num1 + (num2 << 32)) / 10000) - 11644473600000;
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
    const raw = await sendRdsCommand(this.ctx, "FILEIO", [filepath, "READ", ""]);
    const [, offset] = parseNumber(raw, 0);
    const [dataBytes, o1] = parseBytearray(raw, offset);
    const [modified, o2] = parseString(raw, o1);
    const [permission] = parseString(raw, o2);
    return { data: dataBytes, size: dataBytes.length, modified, permission };
  }

  async fileWrite(filepath: string, content: string | Buffer): Promise<void> {
    const data = typeof content === "string" ? Buffer.from(content, "utf-8") : content;
    await sendRdsCommand(this.ctx, "FILEIO", [filepath, "WRITE", "", data]);
  }

  async fileRename(filepathFrom: string, filepathTo: string): Promise<void> {
    await sendRdsCommand(this.ctx, "FILEIO", [filepathFrom, "RENAME", "", filepathTo]);
  }

  async fileRemove(filepath: string): Promise<void> {
    await sendRdsCommand(this.ctx, "FILEIO", [filepath, "REMOVE", "", "F"]);
  }

  async dirRemove(dirpath: string): Promise<void> {
    await sendRdsCommand(this.ctx, "FILEIO", [dirpath, "REMOVE", "", "D"]);
  }

  async fileExists(pathname: string): Promise<boolean> {
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
    await sendRdsCommand(this.ctx, "DBGREQUEST", ["DBG_STOP", sessionName]);
  }

  async debuggerGetServerInfo(sessionName?: string): Promise<number> {
    let autoSession = false;
    let name = sessionName;
    if (!name) {
      name = await this.debuggerStart();
      autoSession = true;
    }
    try {
      const raw = await sendRdsCommand(this.ctx, "DBGREQUEST", ["DBG_GET_DEBUG_SERVER_INFO", name]);
      const [, offset] = parseNumber(raw, 0);
      const [wddxXml] = parseString(raw, offset);
      const m = wddxXml.match(/<var name=['"]DEBUG_SERVER_PORT['"]>\s*<number>(\d+(?:\.\d+)?)<\/number>/i);
      return m ? Math.floor(parseFloat(m[1])) : 0;
    } finally {
      if (autoSession) {
        await this.debuggerStop(name);
      }
    }
  }

  async debuggerBreakpointOnException(sessionName: string, enable: boolean): Promise<void> {
    const val = enable ? "true" : "false";
    const wddx = `<wddxPacket version='1.0'><header/><data><array length='1'><struct type='java.util.HashMap'><var name='COMMAND'><string>SESSION_BREAK_ON_EXCEPTION</string></var><var name='BREAK_ON_EXCEPTION'><boolean value='${val}'/></var></struct></array></data></wddxPacket>`;
    await sendRdsCommand(this.ctx, "DBGREQUEST", ["DBG_REQUEST", sessionName, wddx]);
  }

  async debuggerBreakpoint(sessionName: string, filepath: string, line: number, enable: boolean): Promise<void> {
    const cmd = enable ? "SET_BREAKPOINT" : "UNSET_BREAKPOINT";
    const wddx = `<wddxPacket version='1.0'><header/><data><array length='1'><struct type='java.util.HashMap'><var name='COMMAND'><string>${cmd}</string></var><var name='FILE'><string>${filepath}</string></var><var name='Y'><number>${line}</number></var><var name='SEQ'><number>1.0</number></var></struct></array></data></wddxPacket>`;
    await sendRdsCommand(this.ctx, "DBGREQUEST", ["DBG_REQUEST", sessionName, wddx]);
  }

  async debuggerClearAllBreakpoints(sessionName: string): Promise<void> {
    const wddx = "<wddxPacket version='1.0'><header/><data><array length='1'><struct type='java.util.HashMap'><var name='COMMAND'><string>UNSET_ALL_BREAKPOINTS</string></var></struct></array></data></wddxPacket>";
    await sendRdsCommand(this.ctx, "DBGREQUEST", ["DBG_REQUEST", sessionName, wddx]);
  }

  async debuggerGetDebugEvents(sessionName: string): Promise<DebuggerEvent | null> {
    const raw = await sendRdsCommand(this.ctx, "DBGREQUEST", ["DBG_EVENTS", sessionName]);
    const [evtType, offset] = parseNumber(raw, 0);
    let off = offset;

    if (evtType === CFRDS_DEBUGGER_EVENT_TYPE.BREAKPOINT_SET) {
      const [pathname, o1] = parseString(raw, off);
      const [reqLineStr, o2] = parseString(raw, o1);
      const [actLineStr, o3] = parseString(raw, o2);
      off = o3;
      return {
        type: CFRDS_DEBUGGER_EVENT_TYPE.BREAKPOINT_SET,
        data: {
          pathname,
          req_line: /^\d+$/.test(reqLineStr) ? parseInt(reqLineStr, 10) : 0,
          act_line: /^\d+$/.test(actLineStr) ? parseInt(actLineStr, 10) : 0,
        },
      };
    } else if (
      evtType === CFRDS_DEBUGGER_EVENT_TYPE.BREAKPOINT ||
      evtType === CFRDS_DEBUGGER_EVENT_TYPE.STEP
    ) {
      const [source, o1] = parseString(raw, off);
      const [lineStr, o2] = parseString(raw, o1);
      const [threadName, o3] = parseString(raw, o2);
      off = o3;
      return {
        type: evtType as CFRDS_DEBUGGER_EVENT_TYPE.BREAKPOINT | CFRDS_DEBUGGER_EVENT_TYPE.STEP,
        data: {
          source,
          line: /^\d+$/.test(lineStr) ? parseInt(lineStr, 10) : 0,
          thread_name: threadName,
        },
      };
    }
    return null;
  }

  async debuggerAllFetchFlagsEnabled(
    sessionName: string,
    threads: boolean,
    watch: boolean,
    scopes: boolean,
    cfTrace: boolean,
    javaTrace: boolean
  ): Promise<void> {
    const b = (v: boolean): string => (v ? "true" : "false");
    const wddx = `<wddxPacket version='1.0'><header/><data><struct type='java.util.HashMap'>` +
      `<var name='THREADS'><boolean value='${b(threads)}'/></var>` +
      `<var name='WATCH'><boolean value='${b(watch)}'/></var>` +
      `<var name='SCOPES'><boolean value='${b(scopes)}'/></var>` +
      `<var name='CF_TRACE'><boolean value='${b(cfTrace)}'/></var>` +
      `<var name='JAVA_TRACE'><boolean value='${b(javaTrace)}'/></var>` +
      `</struct></data></wddxPacket>`;
    await sendRdsCommand(this.ctx, "DBGREQUEST", ["DBG_EVENTS", sessionName, wddx]);
  }

  async debuggerStepIn(sessionName: string, threadName: string): Promise<void> {
    const wddx = `<wddxPacket version='1.0'><header/><data><array length='1'><struct type='java.util.HashMap'><var name='COMMAND'><string>STEP_IN</string></var><var name='THREAD'><string>${threadName}</string></var></struct></array></data></wddxPacket>`;
    await sendRdsCommand(this.ctx, "DBGREQUEST", ["DBG_REQUEST", sessionName, wddx]);
  }

  async debuggerStepOver(sessionName: string, threadName: string): Promise<void> {
    const wddx = `<wddxPacket version='1.0'><header/><data><array length='1'><struct type='java.util.HashMap'><var name='COMMAND'><string>STEP_OVER</string></var><var name='THREAD'><string>${threadName}</string></var></struct></array></data></wddxPacket>`;
    await sendRdsCommand(this.ctx, "DBGREQUEST", ["DBG_REQUEST", sessionName, wddx]);
  }

  async debuggerStepOut(sessionName: string, threadName: string): Promise<void> {
    const wddx = `<wddxPacket version='1.0'><header/><data><array length='1'><struct type='java.util.HashMap'><var name='COMMAND'><string>STEP_OUT</string></var><var name='THREAD'><string>${threadName}</string></var></struct></array></data></wddxPacket>`;
    await sendRdsCommand(this.ctx, "DBGREQUEST", ["DBG_REQUEST", sessionName, wddx]);
  }

  async debuggerContinue(sessionName: string, threadName: string): Promise<void> {
    const wddx = `<wddxPacket version='1.0'><header/><data><array length='1'><struct type='java.util.HashMap'><var name='COMMAND'><string>CONTINUE</string></var><var name='THREAD'><string>${threadName}</string></var></struct></array></data></wddxPacket>`;
    await sendRdsCommand(this.ctx, "DBGREQUEST", ["DBG_REQUEST", sessionName, wddx]);
  }

  async debuggerWatchExpression(sessionName: string, threadName: string, expression: string): Promise<void> {
    const wddx = `<wddxPacket version='1.0'><header/><data><array length='1'><struct type='java.util.HashMap'><var name='COMMAND'><string>GET_SINGLE_CF_VARIABLE</string></var><var name='VARIABLE_NAME'><string>${expression}</string></var><var name='THREAD'><string>${threadName}</string></var></struct></array></data></wddxPacket>`;
    await sendRdsCommand(this.ctx, "DBGREQUEST", ["DBG_REQUEST", sessionName, wddx]);
  }

  async debuggerSetVariable(sessionName: string, threadName: string, variable: string, value: string): Promise<void> {
    const wddx = `<wddxPacket version='1.0'><header/><data><array length='1'><struct type='java.util.HashMap'><var name='COMMAND'><string>SET_VARIABLE_VALUE</string></var><var name='VARIABLE_NAME'><string>${variable}</string></var><var name='VARIABLE_VALUE'><string>${value}</string></var><var name='THREAD'><string>${threadName}</string></var></struct></array></data></wddxPacket>`;
    await sendRdsCommand(this.ctx, "DBGREQUEST", ["DBG_REQUEST", sessionName, wddx]);
  }

  async debuggerWatchVariables(sessionName: string, variables: string): Promise<void> {
    const vars = variables.split(",").filter((v) => v.trim().length > 0);
    const varTags = vars.map((v, i) => `<var name='WATCH_${i}'><string>${v.trim()}</string></var>`).join("");
    const wddx = `<wddxPacket version='1.0'><header/><data><array length='1'><struct type='java.util.HashMap'><var name='COMMAND'><string>SET_WATCH_VARIABLES</string></var>${varTags}</struct></array></data></wddxPacket>`;
    await sendRdsCommand(this.ctx, "DBGREQUEST", ["DBG_REQUEST", sessionName, wddx]);
  }

  async debuggerGetOutput(sessionName: string, threadName: string): Promise<void> {
    const wddx = `<wddxPacket version='1.0'><header/><data><array length='1'><struct type='java.util.HashMap'><var name='BODY_ONLY'><boolean value='true'/></var><var name='THREAD'><string>${threadName}</string></var></struct></array></data></wddxPacket>`;
    await sendRdsCommand(this.ctx, "DBGREQUEST", ["DBG_REQUEST", sessionName, wddx]);
  }

  async debuggerSetScopeFilter(sessionName: string, filterStr: string): Promise<void> {
    const wddx = `<wddxPacket version='1.0'><header/><data><array length='1'><struct type='java.util.HashMap'><var name='COMMAND'><string>SET_SCOPE_FILTER</string></var><var name='FILTER'><string>${filterStr}</string></var></struct></array></data></wddxPacket>`;
    await sendRdsCommand(this.ctx, "DBGREQUEST", ["DBG_REQUEST", sessionName, wddx]);
  }

  // Security Analyzer Operations
  async securityAnalyzerScan(pathnames: string, recursively: boolean = true, cores: number = 1): Promise<number> {
    try {
      const raw = await sendRdsCommand(this.ctx, "SECURITYANALYZER", [
        "scan", pathnames, recursively ? "true" : "false", String(cores),
      ]);
      const [, offset] = parseNumber(raw, 0);
      const [resStr] = parseString(raw, offset);
      const parsed = JSON.parse(resStr);
      return typeof parsed.id === "number" ? parsed.id : (parseInt(resStr, 10) || 0);
    } catch {
      return 0;
    }
  }

  async securityAnalyzerCancel(commandId: number): Promise<void> {
    await sendRdsCommand(this.ctx, "SECURITYANALYZER", ["cancel", String(commandId)]);
  }

  async securityAnalyzerStatus(commandId: number): Promise<SecurityAnalyzerStatus> {
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
    await sendRdsCommand(this.ctx, "SECURITYANALYZER", ["clean", String(commandId)]);
  }

  // IDE Default
  async ideDefault(version: number = 1): Promise<IdeDefaultResult> {
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
    const raw = await sendRdsCommand(this.ctx, "ADMINAPI", ["cfide.adminapi.debugging", "getlogproperty", logdirectory]);
    const [propStr] = parseString(raw, 0);
    return propStr;
  }

  async adminapiExtensionsGetcustomtagpaths(): Promise<string[]> {
    const raw = await sendRdsCommand(this.ctx, "ADMINAPI", ["cfide.adminapi.extensions", "getcustomtagpaths"]);
    const [cnt, offset] = parseNumber(raw, 0);
    const paths: string[] = [];
    let off = offset;
    for (let i = 0; i < cnt; i++) {
      const [p, o] = parseString(raw, off);
      off = o;
      paths.push(p);
    }
    return paths;
  }

  async adminapiExtensionsSetmapping(name: string, path: string): Promise<void> {
    const wddx = `<wddxPacket version='1.0'><header/><data><struct><var name='${name}'><string>${path}</string></var></struct></data></wddxPacket>`;
    await sendRdsCommand(this.ctx, "ADMINAPI", ["cfide.adminapi.extensions", "setmappings", wddx]);
  }

  async adminapiExtensionsDeletemapping(mapping: string): Promise<void> {
    await sendRdsCommand(this.ctx, "ADMINAPI", ["cfide.adminapi.extensions", "deleltemappings", mapping]);
  }

  async adminapiExtensionsGetmappings(): Promise<Record<string, string>> {
    const raw = await sendRdsCommand(this.ctx, "ADMINAPI", ["cfide.adminapi.extensions", "getmappings"]);
    const [cnt, offset] = parseNumber(raw, 0);
    const mappings: Record<string, string> = {};
    let off = offset;
    for (let i = 0; i < cnt; i++) {
      const [k, o1] = parseString(raw, off);
      const [v, o2] = parseString(raw, o1);
      off = o2;
      mappings[k] = v;
    }
    return mappings;
  }

  // Graphing Operations
  async graphing(chartAttributes: string, seriesData: string[]): Promise<Buffer> {
    const args: (string | Buffer)[] = ["GRAPH", chartAttributes, String(seriesData.length), ...seriesData];
    return sendRdsCommand(this.ctx, "GRAPHING", args);
  }
}

export { Server as ServerType };
