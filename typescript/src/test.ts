import {
  CFRDS_STATUS,
  CFRDS_DEBUGGER_EVENT_TYPE,
  CFRDSError,
  Server,
  encodePassword,
  parseStringListItem,
} from "./index";

function assert(condition: boolean, message: string): void {
  if (!condition) {
    throw new Error(`ASSERT FAILED: ${message}`);
  }
}

function log(msg: string): void {
  console.log(msg);
}

async function main(): Promise<void> {
  // Collect RDS env vars
  const rdsVars: Record<string, string> = {};
  for (const [k, v] of Object.entries(process.env)) {
    if (k.startsWith("RDS_") && v !== undefined) {
      rdsVars[k] = v;
    }
  }
  if (Object.keys(rdsVars).length > 0) {
    log("RDS environment variables:");
    for (const k of Object.keys(rdsVars).sort()) {
      log(`  ${k}=${rdsVars[k]}`);
    }
  } else {
    log("No RDS_* environment variables set.");
  }

  log("Testing pure TypeScript cfrds module...");

  // Verify status enums and constants
  assert(CFRDS_STATUS.OK === 0, "CFRDS_STATUS.OK should be 0");
  assert(CFRDS_STATUS.COMMAND_FAILED === 6, "CFRDS_STATUS.COMMAND_FAILED should be 6");
  assert(
    CFRDS_DEBUGGER_EVENT_TYPE.BREAKPOINT === 1,
    "CFRDS_DEBUGGER_EVENT_TYPE.BREAKPOINT should be 1"
  );
  assert(
    CFRDS_DEBUGGER_EVENT_TYPE.BREAKPOINT_SET === 0,
    "CFRDS_DEBUGGER_EVENT_TYPE.BREAKPOINT_SET should be 0"
  );

  // Verify CFRDSError works
  const err = new CFRDSError(CFRDS_STATUS.COMMAND_FAILED, "test error");
  assert(err.status === CFRDS_STATUS.COMMAND_FAILED, "CFRDSError.status should match");
  assert(err.message.includes("COMMAND_FAILED"), "CFRDSError message should contain status name");
  assert(err.message.includes("test error"), "CFRDSError message should contain custom message");
  assert(err.name === "CFRDSError", "CFRDSError.name should be 'CFRDSError'");

  // Verify Server class exists and has all expected methods
  const serverMethods = [
    "browseDir",
    "fileRead",
    "fileWrite",
    "fileRename",
    "fileRemove",
    "dirRemove",
    "fileExists",
    "dirCreate",
    "cfRootDir",
    "sqlDsninfo",
    "sqlTableinfo",
    "sqlColumninfo",
    "sqlPrimarykeys",
    "sqlForeignkeys",
    "sqlImportedkeys",
    "sqlExportedkeys",
    "sqlSqlstmnt",
    "sqlMetadata",
    "sqlGetsupportedcommands",
    "sqlDbdescription",
    "debuggerStart",
    "debuggerStop",
    "debuggerGetServerInfo",
    "debuggerBreakpointOnException",
    "debuggerBreakpoint",
    "debuggerClearAllBreakpoints",
    "debuggerGetDebugEvents",
    "debuggerAllFetchFlagsEnabled",
    "debuggerStepIn",
    "debuggerStepOver",
    "debuggerStepOut",
    "debuggerContinue",
    "debuggerWatchExpression",
    "debuggerSetVariable",
    "debuggerWatchVariables",
    "debuggerGetOutput",
    "debuggerSetScopeFilter",
    "securityAnalyzerScan",
    "securityAnalyzerCancel",
    "securityAnalyzerStatus",
    "securityAnalyzerResult",
    "securityAnalyzerClean",
    "ideDefault",
    "adminapiDebuggingGetlogproperty",
    "adminapiExtensionsGetcustomtagpaths",
    "adminapiExtensionsSetmapping",
    "adminapiExtensionsDeletemapping",
    "adminapiExtensionsGetmappings",
    "graphing",
  ];

  for (const method of serverMethods) {
    assert(
      typeof (Server.prototype as any)[method] === "function",
      `Server.prototype.${method} should be a function`
    );
  }

  log(`All ${serverMethods.length} Server methods successfully verified!`);

  // Verify utility functions
  assert(typeof encodePassword === "function", "encodePassword should be exported");
  assert(typeof parseStringListItem === "function", "parseStringListItem should be exported");

  // Verify encodePassword produces correct output
  // "admin" XOR "4p0L@r1$" should produce a known hex string
  const encoded = encodePassword("admin");
  assert(encoded.length > 0, "encodePassword should produce non-empty output");
  assert(/^[0-9a-f]+$/.test(encoded), "encodePassword output should be hex");

  // Verify password round-trip with C implementation
  const expectedAdmin = "55145d252e";
  assert(encoded === expectedAdmin, `encodePassword('admin') should be '${expectedAdmin}', got '${encoded}'`);

  // Verify parseStringListItem
  const items1 = parseStringListItem('"a","b","c"');
  assert(items1.length === 3, "parseStringListItem should parse 3 items");
  assert(items1[0] === "a", "first item should be 'a'");
  assert(items1[1] === "b", "second item should be 'b'");
  assert(items1[2] === "c", "third item should be 'c'");

  const items2 = parseStringListItem("noquotes,here");
  assert(items2.length === 2, "parseStringListItem should parse unquoted items");
  assert(items2[0] === "noquotes", "first item should be 'noquotes'");
  assert(items2[1] === "here", "second item should be 'here'");

  const items3 = parseStringListItem('"quoted with spaces",simple');
  assert(items3.length === 2, "parseStringListItem should handle spaces in quotes");
  assert(items3[0] === "quoted with spaces", "should preserve spaces");
  assert(items3[1] === "simple", "second item should be 'simple'");

  log("Utility function tests passed!");

  // Verify Server constructor defaults
  const srv = new Server("192.168.1.100", 8501, "testuser", "testpass");
  assert(srv.getHost() === "192.168.1.100", "getHost should return constructor value");
  assert(srv.getPort() === 8501, "getPort should return constructor value");
  assert(srv.getUsername() === "testuser", "getUsername should return constructor value");
  assert(srv.getPassword() === "testpass", "getPassword should return constructor value");
  assert(srv.getError() === null, "getError should be null initially");
  assert(srv.getErrorCode() === 1, "getErrorCode should be 1 initially");

  srv.clearError();
  assert(srv.getError() === null, "clearError should reset error");

  log("Server constructor tests passed!");

  // Live server integration tests if env vars present
  const rdsHost = process.env.RDS_HOST;
  const rdsPort = process.env.RDS_PORT;
  const rdsUsername = process.env.RDS_USERNAME || "admin";
  const rdsPassword = process.env.RDS_PASSWORD || "";

  if (rdsHost && rdsPort) {
    const port = parseInt(rdsPort, 10);
    log(`Connecting to live RDS server at ${rdsHost}:${port}...`);
    const rds = new Server(rdsHost, port, rdsUsername, rdsPassword);

    // CF Root Dir
    try {
      log("Getting CF root dir...");
      const rootDir = await rds.cfRootDir();
      log(`CF Root Dir: ${rootDir}`);
    } catch (e) {
      log(`CF Root Dir error: ${e}`);
    }

    // Browse Directory
    try {
      log("Browsing directory...");
      const items = await rds.browseDir("/");
      log(`Found ${items.length} items in /`);
    } catch (e) {
      log(`Browse dir error: ${e}`);
    }

    // Debugger
    try {
      log("Starting Debugger...");
      const sessionId = await rds.debuggerStart();
      log(`Debugger session ID: ${sessionId}`);
      await rds.debuggerStop(sessionId);
      log("Debugger session stopped.");
    } catch (e) {
      log(`Debugger notification (expected if CF debugging disabled): ${e}`);
    }

    // Graphing
    try {
      log("Testing graphing (bar chart)...");
      const chartAttrs = "type=bar;height=400;width=600;title=Test Chart";
      const series = ["seriesLabel=Sales;values=10,20,30,40"];
      const imgBytes = await rds.graphing(chartAttrs, series);
      const hexPrefix = imgBytes.subarray(0, 8).toString("hex");
      log(`Graph rendered successfully (${imgBytes.length} bytes, starts with: ${hexPrefix})`);
    } catch (e) {
      log(`Graphing error: ${e}`);
    }
  }

  log("cfrds pure TypeScript test completed successfully!");
}

main().catch((e) => {
  console.error(e);
  process.exit(1);
});
