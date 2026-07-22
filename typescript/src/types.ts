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
  constructor(message: string = "") {
    super(message);
    this.name = "CFRDSError";
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
  error: string | null;
  agent?: http.Agent;
}

