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

export const CFRDS_STATUS_OK = CFRDS_STATUS.OK;
export const CFRDS_STATUS_MEMORY_ERROR = CFRDS_STATUS.MEMORY_ERROR;
export const CFRDS_STATUS_PARAM_IS_NULL = CFRDS_STATUS.PARAM_IS_NULL;
export const CFRDS_STATUS_SERVER_IS_NULL = CFRDS_STATUS.SERVER_IS_NULL;
export const CFRDS_STATUS_INVALID_INPUT_PARAMETER = CFRDS_STATUS.INVALID_INPUT_PARAMETER;
export const CFRDS_STATUS_INDEX_OUT_OF_BOUNDS = CFRDS_STATUS.INDEX_OUT_OF_BOUNDS;
export const CFRDS_STATUS_COMMAND_FAILED = CFRDS_STATUS.COMMAND_FAILED;
export const CFRDS_STATUS_RESPONSE_ERROR = CFRDS_STATUS.RESPONSE_ERROR;
export const CFRDS_STATUS_HTTP_RESPONSE_NOT_FOUND = CFRDS_STATUS.HTTP_RESPONSE_NOT_FOUND;
export const CFRDS_STATUS_DIR_ALREADY_EXISTS = CFRDS_STATUS.DIR_ALREADY_EXISTS;
export const CFRDS_STATUS_SOCKET_HOST_NOT_FOUND = CFRDS_STATUS.SOCKET_HOST_NOT_FOUND;
export const CFRDS_STATUS_SOCKET_CREATION_FAILED = CFRDS_STATUS.SOCKET_CREATION_FAILED;
export const CFRDS_STATUS_CONNECTION_TO_SERVER_FAILED = CFRDS_STATUS.CONNECTION_TO_SERVER_FAILED;
export const CFRDS_STATUS_WRITING_TO_SOCKET_FAILED = CFRDS_STATUS.WRITING_TO_SOCKET_FAILED;
export const CFRDS_STATUS_PARTIALLY_WRITE_TO_SOCKET = CFRDS_STATUS.PARTIALLY_WRITE_TO_SOCKET;
export const CFRDS_STATUS_READING_FROM_SOCKET_FAILED = CFRDS_STATUS.READING_FROM_SOCKET_FAILED;
export const CFRDS_STATUS_RESPONSE_TOO_LARGE = CFRDS_STATUS.RESPONSE_TOO_LARGE;

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

export interface CFRDSErrorOptions extends ErrorOptions {
  status?: CFRDS_STATUS | number;
  code?: string;
}

export class CFRDSError extends Error {
  private _status?: CFRDS_STATUS | number;
  readonly code?: string;

  constructor(message: string = "", optionsOrStatus?: CFRDSErrorOptions | CFRDS_STATUS | number) {
    let options: CFRDSErrorOptions | undefined;
    let explicitStatus: CFRDS_STATUS | number | undefined;

    if (typeof optionsOrStatus === "number") {
      explicitStatus = optionsOrStatus;
    } else if (optionsOrStatus && typeof optionsOrStatus === "object") {
      options = optionsOrStatus;
      explicitStatus = options.status;
    }

    super(message, options);
    this.name = "CFRDSError";
    this._status = explicitStatus;
    this.code = options?.code;
    Object.setPrototypeOf(this, new.target.prototype);
  }

  get status(): CFRDS_STATUS | number {
    if (this._status !== undefined) {
      return this._status;
    }
    const msg = this.message || "";
    if (msg.includes("is required") || msg.includes("PARAM_IS_NULL")) {
      return CFRDS_STATUS.PARAM_IS_NULL;
    }
    if (msg.includes("Invalid total items count") || msg.includes("RESPONSE_ERROR")) {
      return CFRDS_STATUS.RESPONSE_ERROR;
    }
    if (msg.includes("Socket host not found")) {
      return CFRDS_STATUS.SOCKET_HOST_NOT_FOUND;
    }
    if (msg.includes("Socket creation failed")) {
      return CFRDS_STATUS.SOCKET_CREATION_FAILED;
    }
    if (msg.includes("Connection to server failed") || msg.includes("Connection timed out")) {
      return CFRDS_STATUS.CONNECTION_TO_SERVER_FAILED;
    }
    if (msg.includes("Response too large")) {
      return CFRDS_STATUS.RESPONSE_TOO_LARGE;
    }
    if (msg.includes("Reading from socket failed")) {
      return CFRDS_STATUS.READING_FROM_SOCKET_FAILED;
    }
    if (msg.includes("COMMAND_FAILED")) {
      return CFRDS_STATUS.COMMAND_FAILED;
    }
    return CFRDS_STATUS.COMMAND_FAILED;
  }
}

export class CFRDSNetworkError extends CFRDSError {
  constructor(
    message: string = "Network error",
    optionsOrStatus?: CFRDSErrorOptions | CFRDS_STATUS | number
  ) {
    const defaultStatus = CFRDS_STATUS.CONNECTION_TO_SERVER_FAILED;
    let options: CFRDSErrorOptions;
    if (typeof optionsOrStatus === "number") {
      options = { status: optionsOrStatus };
    } else {
      options = { status: defaultStatus, ...optionsOrStatus };
    }
    super(message, options);
    this.name = "CFRDSNetworkError";
    Object.setPrototypeOf(this, new.target.prototype);
  }
}

export class CFRDSResponseError extends CFRDSError {
  constructor(
    message: string = "Response error",
    optionsOrStatus?: CFRDSErrorOptions | CFRDS_STATUS | number
  ) {
    const defaultStatus = CFRDS_STATUS.RESPONSE_ERROR;
    let options: CFRDSErrorOptions;
    if (typeof optionsOrStatus === "number") {
      options = { status: optionsOrStatus };
    } else {
      options = { status: defaultStatus, ...optionsOrStatus };
    }
    super(message, options);
    this.name = "CFRDSResponseError";
    Object.setPrototypeOf(this, new.target.prototype);
  }
}

export class CFRDSCommandError extends CFRDSError {
  constructor(
    message: string = "Command failed",
    optionsOrStatus?: CFRDSErrorOptions | CFRDS_STATUS | number
  ) {
    const defaultStatus = CFRDS_STATUS.COMMAND_FAILED;
    let options: CFRDSErrorOptions;
    if (typeof optionsOrStatus === "number") {
      options = { status: optionsOrStatus };
    } else {
      options = { status: defaultStatus, ...optionsOrStatus };
    }
    super(message, options);
    this.name = "CFRDSCommandError";
    Object.setPrototypeOf(this, new.target.prototype);
  }
}

export class CFRDSValidationError extends CFRDSError {
  constructor(
    message: string = "Validation failed",
    optionsOrStatus?: CFRDSErrorOptions | CFRDS_STATUS | number
  ) {
    const defaultStatus = CFRDS_STATUS.PARAM_IS_NULL;
    let options: CFRDSErrorOptions;
    if (typeof optionsOrStatus === "number") {
      options = { status: optionsOrStatus };
    } else {
      options = { status: defaultStatus, ...optionsOrStatus };
    }
    super(message, options);
    this.name = "CFRDSValidationError";
    Object.setPrototypeOf(this, new.target.prototype);
  }
}

import * as http from "http";

export interface ServerConfig {
  host: string;
  port: number;
  username: string;
  password: string;
}

export interface ServerContext {
  config: ServerConfig;
  encodedPassword: string;
  agent?: http.Agent;
}

