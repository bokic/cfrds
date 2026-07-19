export enum CFRDS_STATUS {
  OK = 0,
  MEMORY_ERROR = 1,
  PARAM_IS_NULL = 2,
  SERVER_IS_NULL = 3,
  INVALID_INPUT_PARAMETER = 4,
  INDEX_OUT_OF_BOUNDS = 5,
  COMMAND_FAILED = 6,
  RESPONSE_ERROR = 7,
  HTTP_RESPONSE_NOT_FOUND = 8,
  DIR_ALREADY_EXISTS = 9,
  SOCKET_HOST_NOT_FOUND = 10,
  SOCKET_CREATION_FAILED = 11,
  CONNECTION_TO_SERVER_FAILED = 12,
  WRITING_TO_SOCKET_FAILED = 13,
  PARTIALLY_WRITE_TO_SOCKET = 14,
  READING_FROM_SOCKET_FAILED = 15,
  RESPONSE_TOO_LARGE = 16,
}

export enum CFRDS_DEBUGGER_EVENT_TYPE {
  BREAKPOINT_SET = 0,
  BREAKPOINT = 1,
  STEP = 2,
  UNKNOWN = 3,
}

export interface BrowseDirItem {
  kind: string;
  name: string;
  permissions: string;
  size: number;
  modified: number;
}

export interface FileContent {
  data: Buffer;
  size: number;
  modified: string;
  permission: string;
}

export interface SqlTableInfoItem {
  unknown: string;
  schema: string;
  name: string;
  type: string;
}

export interface SqlColumnInfoItem {
  schema: string;
  owner: string;
  table: string;
  name: string;
  type: number;
  typeStr: string;
  precision: number;
  length: number;
  scale: number;
  radix: number;
  nullable: number;
}

export interface SqlPrimaryKeyItem {
  catalog: string;
  owner: string;
  table: string;
  column: string;
  key_sequence: number;
}

export interface SqlForeignKeyItem {
  pkcatalog: string;
  pkowner: string;
  pktable: string;
  pkcolumn: string;
  fkcatalog: string;
  fkowner: string;
  fktable: string;
  fkcolumn: string;
  key_sequence: number;
  updaterule: number;
  deleterule: number;
}

export interface SqlResultSet {
  columns: number;
  rows: number;
  names: string[];
  values: (string | null)[][];
}

export interface SqlMetadataItem {
  name: string;
  type: string;
  jtype: string;
}

export interface DebuggerEvent {
  type: CFRDS_DEBUGGER_EVENT_TYPE;
  data: Record<string, unknown>;
}

export interface SecurityAnalyzerResult {
  data: Record<string, unknown>;
}

export interface AdminApiCustomTagPaths {
  paths: string[];
}

export interface AdminApiMappings {
  mappings: Record<string, string>;
  keys: string[];
  values: string[];
}

export interface IdeDefaultResult {
  num1: number;
  server_version: string;
  client_version: string;
  num2: number;
  num3: number;
}

export interface SecurityAnalyzerStatus {
  totalfiles: number;
  filesvisitedcount: number;
  percentage: number;
  lastupdated: number;
}

export class CFRDSError extends Error {
  status: CFRDS_STATUS;

  constructor(status: CFRDS_STATUS, message: string = "") {
    let statusName: string;
    try {
      statusName = CFRDS_STATUS[status];
    } catch {
      statusName = `STATUS_${status}`;
    }
    const fullMsg = message ? `${statusName} (${status}): ${message}` : `${statusName} (${status})`;
    super(fullMsg);
    this.status = status;
    this.name = "CFRDSError";
  }
}

export interface ServerConfig {
  host: string;
  port: number;
  username: string;
  password: string;
}

export interface ServerContext {
  config: ServerConfig;
  encodedPassword: string;
  errorCode: number;
  error: string | null;
}
