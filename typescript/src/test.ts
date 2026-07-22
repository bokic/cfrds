import {
  CFRDS_STATUS,
  CFRDS_DEBUGGER_EVENT_TYPE,
  CFRDSError,
  Server,
  VERSION,
  cfrds_debugger_event_get_type,
  cfrds_debugger_event_breakpoint_get_source,
  cfrds_debugger_event_breakpoint_get_line,
  cfrds_debugger_event_breakpoint_get_scopes,
  cfrds_debugger_event_breakpoint_get_thread_name,
  cfrds_debugger_event_breakpoint_set_get_pathname,
  cfrds_debugger_event_breakpoint_set_get_req_line,
  cfrds_debugger_event_breakpoint_set_get_act_line,
  cfrds_debugger_event_get_scopes_count,
  cfrds_debugger_event_get_scopes_item,
  cfrds_debugger_event_get_threads_count,
  cfrds_debugger_event_get_threads_item,
  cfrds_debugger_event_get_watch_count,
  cfrds_debugger_event_get_watch_item,
  cfrds_debugger_event_get_cf_trace_count,
  cfrds_debugger_event_get_cf_trace_item,
  cfrds_debugger_event_get_java_trace_count,
  cfrds_debugger_event_get_java_trace_item,
} from "./index";
import { encodePassword, parseStringListItem, wddxDeserialize } from "./parser";
import * as http from "http";
import { EventEmitter } from "events";

declare const require: any;

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

  // Verify VERSION
  assert(typeof VERSION === "string" && VERSION.length > 0, "VERSION should be a non-empty string");
  log(`Module version: ${VERSION}`);

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

  // Verify Server class exists and has all expected methods on its prototype
  const serverMethods = [
    "getHost",
    "getPort",
    "getUsername",
    "getPassword",
    "getError",
    "getErrorCode",
    "clearError",
    "close",
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
  ];

  for (const method of serverMethods) {
    assert(
      typeof (Server.prototype as any)[method] === "function",
      `Server.prototype.${method} should be a function`
    );
  }

  log(`All ${serverMethods.length} Server methods successfully verified on prototype!`);

  // Verify utility functions
  assert(typeof encodePassword === "function", "encodePassword should be exported");
  assert(typeof parseStringListItem === "function", "parseStringListItem should be exported");

  // Verify encodePassword produces correct output
  const encoded = encodePassword("admin");
  assert(encoded.length > 0, "encodePassword should produce non-empty output");
  assert(/^[0-9a-f]+$/.test(encoded), "encodePassword output should be hex");

  // Verify password round-trip with expected hash
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

  // Verify Server constructor defaults and getter/setter/state methods
  const srv = new Server("192.168.1.100", 8501, "testuser", "testpass");
  assert(srv.getHost() === "192.168.1.100", "getHost should return constructor value");
  assert(srv.getPort() === 8501, "getPort should return constructor value");
  assert(srv.getUsername() === "testuser", "getUsername should return constructor value");
  assert(srv.getPassword() === "testpass", "getPassword should return constructor value");
  assert(srv.getError() === null, "getError should be null initially");
  assert(srv.getErrorCode() === 1, "getErrorCode should be 1 initially");

  srv.clearError();
  assert(srv.getError() === null, "clearError should reset error");

  await srv.close();

  log("Server constructor and state method tests passed!");

  // Verify browseDir validation (offline tests using http mock)
  {
    const httpModule = require("http");
    const originalRequest = httpModule.request;
    let mockResponseBody: Buffer = Buffer.from("0:", "utf-8");
    let lastRequestBody = "";

    httpModule.request = (options: any, callback: any) => {
      const mockRes = new EventEmitter() as any;
      mockRes.statusCode = 200;
      mockRes.statusMessage = "OK";
      
      const mockReq = new EventEmitter() as any;
      mockReq.write = (chunk: any) => {
        if (chunk) {
          lastRequestBody += chunk.toString();
        }
      };
      mockReq.end = () => {
        process.nextTick(() => {
          callback(mockRes);
          mockRes.emit("data", mockResponseBody);
          mockRes.emit("end");
        });
      };
      return mockReq;
    };

    try {
      const srvMock = new Server("127.0.0.1", 8500, "admin", "admin");
      
      // Test 1: total = 0 (divisible by 5) -> should succeed
      mockResponseBody = Buffer.from("0:", "utf-8");
      const items0 = await srvMock.browseDir("/");
      assert(items0.length === 0, "browseDir with total = 0 should return empty list");

      // Test 2: total = 3 (not divisible by 5) -> should throw CFRDSError with RESPONSE_ERROR
      mockResponseBody = Buffer.from("3:", "utf-8");
      let threw = false;
      try {
        await srvMock.browseDir("/");
      } catch (e: any) {
        if (e instanceof CFRDSError && e.status === CFRDS_STATUS.RESPONSE_ERROR) {
          threw = true;
        } else {
          throw new Error(`Unexpected error thrown: ${e}`);
        }
      }
      assert(threw, "browseDir should throw CFRDSError(RESPONSE_ERROR) when total is 3");

      log("Offline browseDir validation tests passed!");

      // Test 3: WDDX XML escaping in debuggerBreakpoint
      lastRequestBody = "";
      mockResponseBody = Buffer.from("0:", "utf-8");
      await srvMock.debuggerBreakpoint("mysession", "foo<bar>&baz", 42, true);
      assert(lastRequestBody.includes("<string>foo&lt;bar&gt;&amp;baz</string>"), "debuggerBreakpoint filepath should be XML-escaped");

      // Test 4: WDDX XML escaping in debuggerWatchExpression
      lastRequestBody = "";
      mockResponseBody = Buffer.from("0:", "utf-8");
      await srvMock.debuggerWatchExpression("mysession", "thread<1>", "expr&val");
      assert(lastRequestBody.includes("<string>expr&amp;val</string>"), "debuggerWatchExpression expression should be XML-escaped");
      assert(lastRequestBody.includes("<string>thread&lt;1&gt;</string>"), "debuggerWatchExpression threadName should be XML-escaped");

      // Test 5: WDDX XML escaping in adminapiExtensionsSetmapping
      lastRequestBody = "";
      mockResponseBody = Buffer.from("0:", "utf-8");
      await srvMock.adminapiExtensionsSetmapping("map'name", "path\"val");
      assert(lastRequestBody.includes("<var name='map&apos;name'>"), "adminapiExtensionsSetmapping name should be XML-escaped");
      // Test 6: wddxDeserialize offline validation
      {
        const wddx = `<wddxPacket version='1.0'>
          <header/>
          <data>
            <struct>
              <var name='nullVal'><null/></var>
              <var name='boolTrue'><boolean value='true'/></var>
              <var name='boolFalse'><boolean value='false'/></var>
              <var name='numberVal'><number>42.5</number></var>
              <var name='stringVal'><string>hello &lt;world&gt; &amp; &quot;everyone&quot;</string></var>
              <var name='cdataVal'><string><![CDATA[cdata <test> & val]]></string></var>
              <var name='arrayVal'>
                <array length='2'>
                  <string>item1</string>
                  <struct>
                    <var name='nestedKey'><string>nestedVal</string></var>
                  </struct>
                </array>
              </var>
            </struct>
          </data>
        </wddxPacket>`;
        const parsed = wddxDeserialize(wddx);
        assert(parsed !== null, "parsed should not be null");
        assert(parsed.nullVal === null, "nullVal should be null");
        assert(parsed.boolTrue === true, "boolTrue should be true");
        assert(parsed.boolFalse === false, "boolFalse should be false");
        assert(parsed.numberVal === 42.5, "numberVal should be 42.5");
        assert(parsed.stringVal === 'hello <world> & "everyone"', "stringVal should have unescaped XML entities");
        assert(parsed.cdataVal === 'cdata <test> & val', "cdataVal should parse CDATA literally");
        assert(Array.isArray(parsed.arrayVal), "arrayVal should be an array");
        assert(parsed.arrayVal.length === 2, "arrayVal length should be 2");
        assert(parsed.arrayVal[0] === 'item1', "arrayVal[0] should be item1");
        assert(parsed.arrayVal[1].nestedKey === 'nestedVal', "arrayVal[1].nestedKey should be nestedVal");
        log("Offline wddxDeserialize tests passed!");
      }

      // Test 7: Debugger event accessors offline validation
      {
        const mockEvent = {
          type: CFRDS_DEBUGGER_EVENT_TYPE.BREAKPOINT,
          data: {
            source: "/app/index.cfm",
            line: 42,
            thread_name: "my-thread",
            SCOPES: ["Variables", "Session"],
            THREADS: ["my-thread", "other-thread"],
            WATCH: ["expr1", "expr2"],
            CF_TRACE: ["trace1", "trace2"],
            JAVA_TRACE: ["jtrace1", "jtrace2"]
          }
        };

        assert(cfrds_debugger_event_get_type(mockEvent) === CFRDS_DEBUGGER_EVENT_TYPE.BREAKPOINT, "get_type mismatch");
        assert(cfrds_debugger_event_breakpoint_get_source(mockEvent) === "/app/index.cfm", "get_source mismatch");
        assert(cfrds_debugger_event_breakpoint_get_line(mockEvent) === 42, "get_line mismatch");
        assert(cfrds_debugger_event_breakpoint_get_thread_name(mockEvent) === "my-thread", "get_thread_name mismatch");
        
        const scopes = cfrds_debugger_event_breakpoint_get_scopes(mockEvent);
        assert(Array.isArray(scopes) && scopes[0] === "Variables", "get_scopes mismatch");

        assert(cfrds_debugger_event_get_scopes_count(mockEvent) === 2, "scopes count mismatch");
        assert(cfrds_debugger_event_get_scopes_item(mockEvent, 0) === "Variables", "scopes item 0 mismatch");
        assert(cfrds_debugger_event_get_scopes_item(mockEvent, 1) === "Session", "scopes item 1 mismatch");
        assert(cfrds_debugger_event_get_scopes_item(mockEvent, 2) === null, "scopes item out of bounds mismatch");

        assert(cfrds_debugger_event_get_threads_count(mockEvent) === 2, "threads count mismatch");
        assert(cfrds_debugger_event_get_threads_item(mockEvent, 0) === "my-thread", "threads item 0 mismatch");

        assert(cfrds_debugger_event_get_watch_count(mockEvent) === 2, "watch count mismatch");
        assert(cfrds_debugger_event_get_watch_item(mockEvent, 0) === "expr1", "watch item 0 mismatch");

        assert(cfrds_debugger_event_get_cf_trace_count(mockEvent) === 2, "cf_trace count mismatch");
        assert(cfrds_debugger_event_get_cf_trace_item(mockEvent, 0) === "trace1", "cf_trace item 0 mismatch");

        assert(cfrds_debugger_event_get_java_trace_count(mockEvent) === 2, "java_trace count mismatch");
        assert(cfrds_debugger_event_get_java_trace_item(mockEvent, 0) === "jtrace1", "java_trace item 0 mismatch");

        log("Offline debugger event accessors tests passed!");
      }

      log("Offline WDDX escaping validation tests passed!");
    } finally {
      httpModule.request = originalRequest;
    }
  }

  // Live server integration tests if env vars present
  const rdsHost = process.env.RDS_HOST;
  const rdsPort = process.env.RDS_PORT;
  const rdsUsername = process.env.RDS_USERNAME || "admin";
  const rdsPassword = process.env.RDS_PASSWORD || "";

  if (rdsHost && rdsPort) {
    const port = parseInt(rdsPort, 10);
    log(`Connecting to live RDS server at ${rdsHost}:${port}...`);
    const rds = new Server(rdsHost, port, rdsUsername, rdsPassword);

    // --- File & Directory Operations ---
    log("\n--- Validating File & Directory Operations ---");
    let cfRoot = "";
    try {
      cfRoot = await rds.cfRootDir();
      log(`  cfRootDir(): ${cfRoot}`);
      assert(typeof cfRoot === "string", "cfRootDir should return a string");
    } catch (e) {
      log(`  cfRootDir error: ${e}`);
    }

    try {
      const targetPath = cfRoot || "/";
      const items = await rds.browseDir(targetPath);
      log(`  browseDir('${targetPath}'): Found ${items.length} items`);
      assert(Array.isArray(items), "browseDir should return an array");
      if (items.length > 0) {
        const sample = items.slice(0, 3);
        log(`  browseDir sample items:`);
        for (const item of sample) {
          log(`    - ${item.name} (${item.kind}, size: ${item.size}, modified: ${new Date(item.modified).toISOString()})`);
          assert(item.modified > 0, "parsed modified date should be positive/valid");
        }
      }
    } catch (e) {
      log(`  browseDir error: ${e}`);
    }

    const testDir = `/tmp/ts_test_dir_${Date.now()}`;
    const testFile = `${testDir}/test.txt`;
    const testFileRenamed = `${testDir}/test_renamed.txt`;

    try {
      await rds.dirCreate(testDir);
      log(`  dirCreate('${testDir}'): success`);

      const existsDir = await rds.fileExists(testDir);
      log(`  fileExists('${testDir}'): ${existsDir}`);
      assert(existsDir === true, "fileExists for created directory should be true");

      const fileData = "Hello ColdFusion RDS TypeScript Test!";
      await rds.fileWrite(testFile, fileData);
      log(`  fileWrite('${testFile}'): success`);

      const existsFile = await rds.fileExists(testFile);
      log(`  fileExists('${testFile}'): ${existsFile}`);
      assert(existsFile === true, "fileExists for written file should be true");

      const readRes = await rds.fileRead(testFile);
      const textRead = readRes.data.toString("utf-8");
      log(`  fileRead('${testFile}'): ${readRes.size} bytes read ("${textRead}")`);
      assert(textRead === fileData, "fileRead data should match fileWrite data");

      await rds.fileRename(testFile, testFileRenamed);
      log(`  fileRename('${testFile}' -> '${testFileRenamed}'): success`);

      const existsOld = await rds.fileExists(testFile);
      assert(existsOld === false, "old file should not exist after rename");

      const existsRenamed = await rds.fileExists(testFileRenamed);
      assert(existsRenamed === true, "renamed file should exist");

      await rds.fileRemove(testFileRenamed);
      log(`  fileRemove('${testFileRenamed}'): success`);

      await rds.dirRemove(testDir);
      log(`  dirRemove('${testDir}'): success`);

      const existsDirAfter = await rds.fileExists(testDir);
      assert(existsDirAfter === false, "directory should not exist after removal");
    } catch (e) {
      log(`  File/Directory operation error: ${e}`);
      try { await rds.fileRemove(testFile); } catch {}
      try { await rds.fileRemove(testFileRenamed); } catch {}
      try { await rds.dirRemove(testDir); } catch {}
    }

    // --- SQL Operations ---
    log("\n--- Validating SQL Operations ---");
    try {
      const supportedCmds = await rds.sqlGetsupportedcommands();
      log(`  sqlGetsupportedcommands(): ${supportedCmds.length} commands (${supportedCmds.join(", ")})`);
      assert(Array.isArray(supportedCmds), "sqlGetsupportedcommands should return an array");
    } catch (e) {
      log(`  sqlGetsupportedcommands error: ${e}`);
    }

    let dsns: string[] = [];
    try {
      dsns = await rds.sqlDsninfo();
      log(`  sqlDsninfo(): ${dsns.length} DSNs found (${dsns.join(", ")})`);
      assert(Array.isArray(dsns), "sqlDsninfo should return an array");
    } catch (e) {
      log(`  sqlDsninfo error: ${e}`);
    }

    const targetDsn = process.env.RDS_DSN || (dsns.length > 0 ? dsns[0] : "");
    if (targetDsn) {
      try {
        const dbDesc = await rds.sqlDbdescription(targetDsn);
        log(`  sqlDbdescription('${targetDsn}'): ${dbDesc}`);
      } catch (e) {
        log(`  sqlDbdescription error: ${e}`);
      }

      let tables: any[] = [];
      try {
        tables = await rds.sqlTableinfo(targetDsn);
        log(`  sqlTableinfo('${targetDsn}'): ${tables.length} tables found`);
        assert(Array.isArray(tables), "sqlTableinfo should return an array");
      } catch (e) {
        log(`  sqlTableinfo error: ${e}`);
      }

      const targetTable = process.env.RDS_DSN_TABLE || (tables.length > 0 ? tables[0].name : "");
      if (targetTable) {
        try {
          const cols = await rds.sqlColumninfo(targetDsn, targetTable);
          log(`  sqlColumninfo('${targetDsn}', '${targetTable}'): ${cols.length} columns`);
          assert(Array.isArray(cols), "sqlColumninfo should return an array");
        } catch (e) {
          log(`  sqlColumninfo error: ${e}`);
        }

        try {
          const pks = await rds.sqlPrimarykeys(targetDsn, targetTable);
          log(`  sqlPrimarykeys('${targetDsn}', '${targetTable}'): ${pks.length} primary keys`);
          assert(Array.isArray(pks), "sqlPrimarykeys should return an array");
        } catch (e) {
          log(`  sqlPrimarykeys error: ${e}`);
        }

        try {
          const fks = await rds.sqlForeignkeys(targetDsn, targetTable);
          log(`  sqlForeignkeys('${targetDsn}', '${targetTable}'): ${fks.length} foreign keys`);
          assert(Array.isArray(fks), "sqlForeignkeys should return an array");
        } catch (e) {
          log(`  sqlForeignkeys error: ${e}`);
        }

        try {
          const impKeys = await rds.sqlImportedkeys(targetDsn, targetTable);
          log(`  sqlImportedkeys('${targetDsn}', '${targetTable}'): ${impKeys.length} imported keys`);
          assert(Array.isArray(impKeys), "sqlImportedkeys should return an array");
        } catch (e) {
          log(`  sqlImportedkeys error: ${e}`);
        }

        try {
          const expKeys = await rds.sqlExportedkeys(targetDsn, targetTable);
          log(`  sqlExportedkeys('${targetDsn}', '${targetTable}'): ${expKeys.length} exported keys`);
          assert(Array.isArray(expKeys), "sqlExportedkeys should return an array");
        } catch (e) {
          log(`  sqlExportedkeys error: ${e}`);
        }
      }

      try {
        const sqlRes = await rds.sqlSqlstmnt(targetDsn, "SELECT 1");
        log(`  sqlSqlstmnt('${targetDsn}', 'SELECT 1'): ${sqlRes.rows} rows, ${sqlRes.columns} columns`);
        assert(Array.isArray(sqlRes.values), "sqlRes.values should be an array");
      } catch (e) {
        log(`  sqlSqlstmnt error: ${e}`);
      }

      try {
        const metaRes = await rds.sqlMetadata(targetDsn, "SELECT 1");
        log(`  sqlMetadata('${targetDsn}', 'SELECT 1'): ${metaRes.length} metadata items`);
        assert(Array.isArray(metaRes), "sqlMetadata should return an array");
      } catch (e) {
        log(`  sqlMetadata error: ${e}`);
      }
    } else {
      log("  Skipping DSN-specific SQL tests (no DSN available)");
    }

    // --- Security Analyzer Operations ---
    log("\n--- Validating Security Analyzer Operations ---");
    try {
      const scanPath = testDir;
      const cmdId = await rds.securityAnalyzerScan(scanPath, true, 1);
      log(`  securityAnalyzerScan('${scanPath}'): commandId=${cmdId}`);
      assert(typeof cmdId === "number", "securityAnalyzerScan should return number");

      if (cmdId > 0) {
        const status = await rds.securityAnalyzerStatus(cmdId);
        log(`  securityAnalyzerStatus(${cmdId}): visited=${status.filesvisitedcount}/${status.totalfiles}`);
        assert(typeof status === "object", "securityAnalyzerStatus should return object");

        const result = await rds.securityAnalyzerResult(cmdId);
        log(`  securityAnalyzerResult(${cmdId}): ${JSON.stringify(result)}`);

        await rds.securityAnalyzerCancel(cmdId);
        log(`  securityAnalyzerCancel(${cmdId}): success`);

        await rds.securityAnalyzerClean(cmdId);
        log(`  securityAnalyzerClean(${cmdId}): success`);
      }
    } catch (e) {
      log(`  Security Analyzer error: ${e}`);
    }

    // --- IDE Default ---
    log("\n--- Validating IDE Default ---");
    try {
      const ideRes = await rds.ideDefault(1);
      log(`  ideDefault(1): server_version=${ideRes.server_version}, client_version=${ideRes.client_version}`);
      assert(typeof ideRes === "object", "ideDefault should return object");
    } catch (e) {
      log(`  ideDefault error: ${e}`);
    }

    // --- Debugging Operations ---
    log("\n--- Debugging Operations ---");
    try {
      log("  Starting Debugger...");
      const sessionId = await rds.debuggerStart();
      log(`  debuggerStart(): sessionId=${sessionId}`);
      assert(typeof sessionId === "string" && sessionId.length > 0, "debuggerStart should return non-empty sessionId string");

      try {
        try {
          const dbgPort = await rds.debuggerGetServerInfo(sessionId);
          log(`  debuggerGetServerInfo('${sessionId}'): port=${dbgPort}`);
        } catch (e) {
          log(`  debuggerGetServerInfo error: ${e}`);
        }

        try {
          await rds.debuggerSetScopeFilter(sessionId, "VARIABLES,SESSION");
          log(`  debuggerSetScopeFilter('${sessionId}'): success`);
        } catch (e) {
          log(`  debuggerSetScopeFilter error: ${e}`);
        }

        try {
          await rds.debuggerWatchVariables(sessionId, "VARIABLES.A");
          log(`  debuggerWatchVariables('${sessionId}'): success`);
        } catch (e) {
          log(`  debuggerWatchVariables error: ${e}`);
        }

        try {
          await rds.debuggerClearAllBreakpoints(sessionId);
          log(`  debuggerClearAllBreakpoints('${sessionId}'): success`);
        } catch (e) {
          log(`  debuggerClearAllBreakpoints error: ${e}`);
        }

        const testCfmContent = "<cfset a = 10>\n<cfset b = 20>\n<cfset c = a + b>\n<cfoutput>Debug Test Page: #c#</cfoutput>\n";
        const appCfmPath = "/app/test_debug.cfm";
        const wwwrootCfmPath = cfRoot ? `${cfRoot}/wwwroot/test_debug.cfm` : "/opt/coldfusion/cfusion/wwwroot/test_debug.cfm";

        try { await rds.fileWrite(appCfmPath, testCfmContent); } catch {}
        try { await rds.fileWrite(wwwrootCfmPath, testCfmContent); } catch {}

        const targetBpPath = appCfmPath;
        try {
          await rds.debuggerBreakpoint(sessionId, targetBpPath, 2, true);
          log(`  debuggerBreakpoint('${sessionId}', '${targetBpPath}', line 2): success`);
        } catch (e) {
          log(`  debuggerBreakpoint error: ${e}`);
        }

        if (wwwrootCfmPath !== appCfmPath) {
          try { await rds.debuggerBreakpoint(sessionId, wwwrootCfmPath, 2, true); } catch {}
        }

        try {
          await rds.debuggerBreakpointOnException(sessionId, true);
          log(`  debuggerBreakpointOnException('${sessionId}'): success`);
        } catch (e) {
          log(`  debuggerBreakpointOnException error: ${e}`);
        }

        try {
          await rds.debuggerAllFetchFlagsEnabled(sessionId, true, true, true, true, true);
          log(`  debuggerAllFetchFlagsEnabled('${sessionId}'): success`);
        } catch (e) {
          log(`  debuggerAllFetchFlagsEnabled error: ${e}`);
        }

        // Fetch Event 1 (Dummy / Breakpoint Registration Event)
        let evt1 = null;
        try {
          evt1 = await rds.debuggerGetDebugEvents(sessionId);
          log(`  >>> EVENT 1 (Dummy / Registration Event): ${JSON.stringify(evt1)}`);
        } catch (e) {
          log(`  debuggerGetDebugEvents (Event 1) error: ${e}`);
        }

        log("\n" + "=".repeat(72));
        log("[DEBUGGER READY]");
        log(`  Created CFML file: ${appCfmPath} (and ${wwwrootCfmPath})`);
        log(`  Breakpoint set on line 2`);
        log(`  Please open http://${rdsHost}:${port}/test_debug.cfm in your browser.`);
        log("  Waiting for Event 2 (Browser Hit Event - script will block until requested)...");
        log("=".repeat(72) + "\n");

        // Fetch Event 2 (Actual Browser Hit Event)
        let evt2 = null;
        try {
          evt2 = await rds.debuggerGetDebugEvents(sessionId);
          log(`  >>> EVENT 2 (Browser Hit Event): ${JSON.stringify(evt2)}`);
        } catch (e) {
          log(`  debuggerGetDebugEvents (Event 2) error: ${e}`);
        }

        const threadName = (evt2 && ((evt2.data as any)?.thread_name || (evt2.data as any)?.thread_id)) ||
                           (evt1 && ((evt1.data as any)?.thread_name || (evt1.data as any)?.thread_id)) || "main";

        try {
          await rds.debuggerWatchExpression(sessionId, threadName, "arrayNew(1)");
          log(`  debuggerWatchExpression('${sessionId}', '${threadName}'): success`);
        } catch (e) {
          log(`  debuggerWatchExpression error: ${e}`);
        }

        try {
          await rds.debuggerSetVariable(sessionId, threadName, "VARIABLES.A", "200");
          log(`  debuggerSetVariable('${sessionId}', '${threadName}'): success`);
        } catch (e) {
          log(`  debuggerSetVariable error: ${e}`);
        }

        try {
          const output = await rds.debuggerGetOutput(sessionId, threadName);
          log(`  debuggerGetOutput('${sessionId}', '${threadName}'): success, output='${output}'`);
        } catch (e) {
          log(`  debuggerGetOutput error: ${e}`);
        }

        try {
          await rds.debuggerStepOver(sessionId, threadName);
          log(`  debuggerStepOver('${sessionId}', '${threadName}'): success`);
        } catch (e) {
          log(`  debuggerStepOver error: ${e}`);
        }

        try {
          await rds.debuggerContinue(sessionId, threadName);
          log(`  debuggerContinue('${sessionId}', '${threadName}'): success`);
        } catch (e) {
          log(`  debuggerContinue error: ${e}`);
        }

        try {
          await rds.debuggerBreakpoint(sessionId, targetBpPath, 2, false);
          log(`  debuggerBreakpoint disable('${sessionId}'): success`);
        } catch (e) {
          log(`  debuggerBreakpoint disable error: ${e}`);
        }
      } finally {
        await rds.debuggerStop(sessionId);
        log(`  debuggerStop('${sessionId}'): success`);
      }
    } catch (e) {
      log(`  Debugger session failed to start (skipping remaining debugger tests): ${e}`);
    }

    await rds.close();
  }

  log("cfrds pure TypeScript test completed successfully!");
}

main().catch((e) => {
  console.error(e);
  process.exit(1);
});
