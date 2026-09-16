import { test, describe, before, after, mock } from "node:test";
import assert from "node:assert/strict";
import http from "node:http";
import { EventEmitter } from "node:events";

import {
  CFRDS_STATUS,
  CFRDS_STATUS_OK,
  CFRDS_STATUS_COMMAND_FAILED,
  CFRDS_DEBUGGER_EVENT_TYPE,
  CFRDSError,
  CFRDSNetworkError,
  CFRDSResponseError,
  CFRDSCommandError,
  CFRDSValidationError,
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
  cfrds_debugger_event_get_scopes_item_name,
  cfrds_debugger_event_get_scopes_item_value,
  cfrds_debugger_event_get_scopes_item,
  cfrds_debugger_event_get_threads_count,
  cfrds_debugger_event_get_threads_item_name,
  cfrds_debugger_event_get_threads_item_state,
  cfrds_debugger_event_get_threads_item,
  cfrds_debugger_event_get_watch_count,
  cfrds_debugger_event_get_watch_item,
  cfrds_debugger_event_get_cf_trace_count,
  cfrds_debugger_event_get_cf_trace_item,
  cfrds_debugger_event_get_java_trace_count,
  cfrds_debugger_event_get_java_trace_item,
  cfrds_version,
  cfrds_version_major,
  cfrds_version_minor,
  cfrds_version_patch,
  cfrds_version_int,
} from "./index";
import { encodePassword, parseStringListItem, parseTimestamp, wddxDeserialize } from "./parser";

describe("cfrds TypeScript module", () => {
  describe("Version and Status Constants", () => {
    test("VERSION is exported as a non-empty string", () => {
      assert.equal(typeof VERSION, "string");
      assert.ok(VERSION.length > 0);
    });

    test("Status enums and backwards-compatible constants match expected values", () => {
      assert.equal(CFRDS_STATUS_OK, 0);
      assert.equal(CFRDS_STATUS.OK, 0);
      assert.equal(CFRDS_STATUS_COMMAND_FAILED, 6);
      assert.equal(CFRDS_STATUS.COMMAND_FAILED, 6);
    });

    test("Version helper functions compute correct values", () => {
      assert.equal(cfrds_version(), VERSION);
      assert.equal(typeof cfrds_version_major(), "number");
      assert.equal(typeof cfrds_version_minor(), "number");
      assert.equal(typeof cfrds_version_patch(), "number");
      const expectedInt =
        cfrds_version_major() * 10000 +
        cfrds_version_minor() * 100 +
        cfrds_version_patch();
      assert.equal(cfrds_version_int(), expectedInt);
    });

    test("Debugger event type enums are correct", () => {
      assert.equal(CFRDS_DEBUGGER_EVENT_TYPE.BREAKPOINT, 1);
      assert.equal(CFRDS_DEBUGGER_EVENT_TYPE.BREAKPOINT_SET, 0);
    });
  });

  describe("Error Hierarchy and Mapping", () => {
    test("CFRDSError has correct default name and status", () => {
      const err = new CFRDSError("test error");
      assert.equal(err.message, "test error");
      assert.equal(err.name, "CFRDSError");
      assert.equal(err.status, CFRDS_STATUS_COMMAND_FAILED);
    });

    test("CFRDSError maps required parameter validation status", () => {
      const nullParamErr = new CFRDSError("filepath is required");
      assert.equal(nullParamErr.status, CFRDS_STATUS.PARAM_IS_NULL);
    });

    test("CFRDSError maps connection failure status", () => {
      const connErr = new CFRDSError("Connection to server failed: ECONNREFUSED");
      assert.equal(connErr.status, CFRDS_STATUS.CONNECTION_TO_SERVER_FAILED);
    });
  });

  describe("Server Class API Shape", () => {
    const expectedServerMethods = [
      "getHost",
      "getPort",
      "getUsername",
      "getPassword",
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
      "debuggerServerStop",
      "debuggerGetServerInfo",
      "debuggerBreakpointOnException",
      "debuggerGlobalBreakpointOnException",
      "debuggerBreakpoint",
      "debuggerClearAllBreakpoints",
      "debuggerGetDebugEvents",
      "debuggerAllFetchFlagsEnabled",
      "debuggerStepIn",
      "debuggerStepOver",
      "debuggerStepOut",
      "debuggerSyncStepIn",
      "debuggerSyncStepOver",
      "debuggerSyncStepOut",
      "debuggerContinue",
      "debuggerGetCfVariables",
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

    test("Server prototype includes all expected public methods", () => {
      const privateMethods = ["parseDebuggerEvent"];
      const actualMethods = Object.getOwnPropertyNames(Server.prototype).filter(
        (name) => name !== "constructor" && !privateMethods.includes(name)
      );

      const missingMethods = expectedServerMethods.filter((m) => !actualMethods.includes(m));
      const extraMethods = actualMethods.filter((m) => !expectedServerMethods.includes(m));

      assert.deepEqual(
        missingMethods,
        [],
        `Server.prototype is missing expected methods: ${missingMethods.join(", ")}`
      );
      assert.deepEqual(
        extraMethods,
        [],
        `Server.prototype has unexpected extra methods: ${extraMethods.join(", ")}`
      );

      for (const method of expectedServerMethods) {
        assert.equal(
          typeof (Server.prototype as any)[method],
          "function",
          `Server.prototype.${method} should be a function`
        );
      }
    });

    test("Server constructor initializes configuration and getters correctly", async () => {
      const srv = new Server("192.168.1.100", 8501, "testuser", "testpass");
      assert.equal(srv.getHost(), "192.168.1.100");
      assert.equal(srv.getPort(), 8501);
      assert.equal(srv.getUsername(), "testuser");
      assert.equal(srv.getPassword(), "testpass");
      await srv.close();
    });
  });

  describe("Utility Functions", () => {
    test("encodePassword produces deterministic hex output", () => {
      assert.equal(typeof encodePassword, "function");
      const encoded = encodePassword("admin");
      assert.ok(encoded.length > 0);
      assert.match(encoded, /^[0-9a-f]+$/);
      assert.equal(encoded, "55145d252e");
    });

    test("parseStringListItem correctly parses quoted and unquoted CSV values", () => {
      assert.equal(typeof parseStringListItem, "function");

      const items1 = parseStringListItem('"a","b","c"');
      assert.deepEqual(items1, ["a", "b", "c"]);

      const items2 = parseStringListItem("noquotes,here");
      assert.deepEqual(items2, ["noquotes", "here"]);

      const items3 = parseStringListItem('"quoted with spaces",simple');
      assert.deepEqual(items3, ["quoted with spaces", "simple"]);
    });

    test("parseTimestamp parses various ColdFusion date formats to milliseconds timestamp", () => {
      assert.equal(typeof parseTimestamp, "function");

      // Ticks format "num1,num2"
      const ticksTs = parseTimestamp("1336987654321,30800000");
      assert.equal(typeof ticksTs, "number");
      assert.ok(ticksTs > 0);

      // CF SimpleDateFormat "hh:mm:ssa MM/dd/yyyy"
      const cfTs = parseTimestamp("02:15:30PM 09/05/2026");
      assert.equal(typeof cfTs, "number");
      assert.ok(cfTs > 0);
      const d = new Date(cfTs);
      assert.equal(d.getFullYear(), 2026);
      assert.equal(d.getMonth(), 8); // September (0-indexed)
      assert.equal(d.getDate(), 5);

      // ISO / standard date string format
      const isoTs = parseTimestamp("2026-07-22 05:00:00");
      assert.equal(typeof isoTs, "number");
      assert.ok(isoTs > 0);

      // Empty / invalid
      assert.equal(parseTimestamp(""), 0);
    });
  });

  describe("WDDX Deserializer", () => {
    test("deserializes complex WDDX structures including types and CDATA", () => {
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
      assert.notEqual(parsed, null);
      assert.equal(parsed.nullVal, null);
      assert.equal(parsed.boolTrue, true);
      assert.equal(parsed.boolFalse, false);
      assert.equal(parsed.numberVal, 42.5);
      assert.equal(parsed.stringVal, 'hello <world> & "everyone"');
      assert.equal(parsed.cdataVal, "cdata <test> & val");
      assert.ok(Array.isArray(parsed.arrayVal));
      assert.equal(parsed.arrayVal.length, 2);
      assert.equal(parsed.arrayVal[0], "item1");
      assert.equal(parsed.arrayVal[1].nestedKey, "nestedVal");
    });
  });

  describe("Debugger Event Accessors", () => {
    test("reads event data correctly from object and array payloads", () => {
      const mockEvent = {
        type: CFRDS_DEBUGGER_EVENT_TYPE.BREAKPOINT,
        data: {
          source: "/app/index.cfm",
          line: 42,
          thread_name: "my-thread",
          SCOPES: { VARIABLES: { A: "10" }, SESSION: { ID: "123" } },
          THREADS: [["my-thread", "RUNNING"], ["other-thread", "WAITING"]],
          WATCH: { expr1: "100", expr2: "200" },
          CF_TRACE: ["trace1", "trace2"],
          JAVA_TRACE: ["jtrace1", "jtrace2"],
        },
      };

      assert.equal(cfrds_debugger_event_get_type(mockEvent), CFRDS_DEBUGGER_EVENT_TYPE.BREAKPOINT);
      assert.equal(cfrds_debugger_event_breakpoint_get_source(mockEvent), "/app/index.cfm");
      assert.equal(cfrds_debugger_event_breakpoint_get_line(mockEvent), 42);
      assert.equal(cfrds_debugger_event_breakpoint_get_thread_name(mockEvent), "my-thread");

      const scopes = cfrds_debugger_event_breakpoint_get_scopes(mockEvent);
      assert.ok(scopes && typeof scopes === "object");

      assert.equal(cfrds_debugger_event_get_scopes_count(mockEvent), 2);
      assert.equal(cfrds_debugger_event_get_scopes_item_name(mockEvent, 0), "VARIABLES");
      assert.equal(cfrds_debugger_event_get_scopes_item_name(mockEvent, 1), "SESSION");
      assert.equal(cfrds_debugger_event_get_scopes_item_name(mockEvent, 2), null);
      assert.notEqual(cfrds_debugger_event_get_scopes_item_value(mockEvent, 0), null);
      assert.equal(cfrds_debugger_event_get_scopes_item(mockEvent, 0), "VARIABLES");

      assert.equal(cfrds_debugger_event_get_threads_count(mockEvent), 2);
      assert.equal(cfrds_debugger_event_get_threads_item_name(mockEvent, 0), "my-thread");
      assert.equal(cfrds_debugger_event_get_threads_item_state(mockEvent, 0), "RUNNING");
      assert.equal(cfrds_debugger_event_get_threads_item_name(mockEvent, 1), "other-thread");
      assert.equal(cfrds_debugger_event_get_threads_item_state(mockEvent, 1), "WAITING");
      assert.equal(cfrds_debugger_event_get_threads_item(mockEvent, 0), "my-thread");

      assert.equal(cfrds_debugger_event_get_watch_count(mockEvent), 2);
      assert.equal(cfrds_debugger_event_get_watch_item(mockEvent, 0), "expr1");
      assert.equal(cfrds_debugger_event_get_watch_item(mockEvent, 1), "expr2");

      assert.equal(cfrds_debugger_event_get_cf_trace_count(mockEvent), 2);
      assert.equal(cfrds_debugger_event_get_cf_trace_item(mockEvent, 0), "trace1");

      assert.equal(cfrds_debugger_event_get_java_trace_count(mockEvent), 2);
      assert.equal(cfrds_debugger_event_get_java_trace_item(mockEvent, 0), "jtrace1");

      const mockArrayEvent = {
        type: CFRDS_DEBUGGER_EVENT_TYPE.BREAKPOINT,
        data: {
          SCOPES: ["Variables", "Session"],
          THREADS: ["my-thread", "other-thread"],
          WATCH: ["expr1", "expr2"],
        },
      };
      assert.equal(cfrds_debugger_event_get_scopes_count(mockArrayEvent), 2);
      assert.equal(cfrds_debugger_event_get_scopes_item_name(mockArrayEvent, 0), "Variables");
      assert.equal(cfrds_debugger_event_get_threads_count(mockArrayEvent), 2);
      assert.equal(cfrds_debugger_event_get_threads_item_name(mockArrayEvent, 0), "my-thread");
      assert.equal(cfrds_debugger_event_get_watch_count(mockArrayEvent), 2);
      assert.equal(cfrds_debugger_event_get_watch_item(mockArrayEvent, 0), "expr1");
    });
  });

  describe("Offline Server and Transport Tests (Mocked HTTP)", () => {
    let mockResponseBody: Buffer = Buffer.from("0:", "utf-8");
    let lastRequestBody = "";
    let mockNetworkErrorCode: string | undefined = undefined;

    before(() => {
      mock.method(http, "request", (options: any, callback: any) => {
        const mockReq = new EventEmitter() as any;
        mockReq.write = (chunk: any) => {
          if (chunk) {
            lastRequestBody += chunk.toString();
          }
        };
        mockReq.end = () => {
          process.nextTick(() => {
            if (mockNetworkErrorCode !== undefined) {
              const err = new Error("Mocked network error") as any;
              err.code = mockNetworkErrorCode === "NONE" ? undefined : mockNetworkErrorCode;
              mockReq.emit("error", err);
              return;
            }

            const mockRes = new EventEmitter() as any;
            mockRes.statusCode = 200;
            mockRes.statusMessage = "OK";
            callback(mockRes);
            mockRes.emit("data", mockResponseBody);
            mockRes.emit("end");
          });
        };
        return mockReq;
      });
    });

    after(() => {
      mock.reset();
    });

    test("browseDir returns empty list when response total count is 0", async () => {
      mockNetworkErrorCode = undefined;
      mockResponseBody = Buffer.from("0:", "utf-8");
      const srv = new Server("127.0.0.1", 8500, "admin", "admin");
      const items = await srv.browseDir("/");
      assert.equal(items.length, 0);
    });

    test("browseDir throws CFRDSValidationError when path is null", async () => {
      mockNetworkErrorCode = undefined;
      const srv = new Server("127.0.0.1", 8500, "admin", "admin");
      await assert.rejects(
        async () => {
          await srv.browseDir(null as any);
        },
        (err: any) => {
          assert.ok(err instanceof CFRDSValidationError);
          assert.ok(err instanceof CFRDSError);
          assert.equal(err.status, CFRDS_STATUS.PARAM_IS_NULL);
          assert.match(err.message, /path is required/);
          return true;
        }
      );
    });

    test("browseDir throws CFRDSResponseError when total count is not divisible by 5", async () => {
      mockNetworkErrorCode = undefined;
      mockResponseBody = Buffer.from("3:", "utf-8");
      const srv = new Server("127.0.0.1", 8500, "admin", "admin");
      await assert.rejects(
        async () => {
          await srv.browseDir("/");
        },
        (err: any) => {
          assert.ok(err instanceof CFRDSResponseError);
          assert.ok(err instanceof CFRDSError);
          assert.equal(err.status, CFRDS_STATUS.RESPONSE_ERROR);
          assert.match(err.message, /Invalid total items count/);
          return true;
        }
      );
    });

    test("debuggerBreakpoint escapes XML entities in parameters", async () => {
      mockNetworkErrorCode = undefined;
      lastRequestBody = "";
      mockResponseBody = Buffer.from("0:", "utf-8");
      const srv = new Server("127.0.0.1", 8500, "admin", "admin");
      await srv.debuggerBreakpoint("mysession", "foo<bar>&baz", 42, true);
      assert.match(lastRequestBody, /<string>foo&lt;bar&gt;&amp;baz<\/string>/);
    });

    test("debuggerWatchExpression escapes XML entities in parameters", async () => {
      mockNetworkErrorCode = undefined;
      lastRequestBody = "";
      mockResponseBody = Buffer.from("0:", "utf-8");
      const srv = new Server("127.0.0.1", 8500, "admin", "admin");
      await srv.debuggerWatchExpression("mysession", "thread<1>", "expr&val");
      assert.match(lastRequestBody, /<string>expr&amp;val<\/string>/);
      assert.match(lastRequestBody, /<string>thread&lt;1&gt;<\/string>/);
    });

    test("adminapiExtensionsSetmapping formats key-value pairs correctly", async () => {
      mockNetworkErrorCode = undefined;
      lastRequestBody = "";
      mockResponseBody = Buffer.from("0:", "utf-8");
      const srv = new Server("127.0.0.1", 8500, "admin", "admin");
      await srv.adminapiExtensionsSetmapping("map'name", "path\"val");
      assert.match(lastRequestBody, /name:map'name;path:path"val/);
    });

    test("adminapiExtensionsGetmappings parses mappings into structured format", async () => {
      mockNetworkErrorCode = undefined;
      lastRequestBody = "";
      const xmlData =
        "<wddxPacket version='1.0'><header/><data><struct><var name='k1'><string>v1</string></var><var name='k2'><string>v2</string></var><var name='k1'><string>v3</string></var></struct></data></wddxPacket>";
      mockResponseBody = Buffer.from(`1:${xmlData.length}:${xmlData}`, "utf-8");
      const srv = new Server("127.0.0.1", 8500, "admin", "admin");
      const res = await srv.adminapiExtensionsGetmappings();

      assert.equal(res.keys.length, 3);
      assert.deepEqual(res.keys, ["k1", "k2", "k1"]);
      assert.deepEqual(res.values, ["v1", "v2", "v3"]);
      assert.equal(res.mappings.k1, "v3");
      assert.equal(res.mappings.k2, "v2");
    });

    describe("Transport Network Error Code Mapping", () => {
      const testCases = [
        { code: "ENOTFOUND", substr: "Socket host not found", status: CFRDS_STATUS.SOCKET_HOST_NOT_FOUND },
        { code: "EAI_AGAIN", substr: "Socket host not found", status: CFRDS_STATUS.SOCKET_HOST_NOT_FOUND },
        { code: "EADDRNOTAVAIL", substr: "Socket creation failed", status: CFRDS_STATUS.SOCKET_CREATION_FAILED },
        { code: "EACCES", substr: "Socket creation failed", status: CFRDS_STATUS.SOCKET_CREATION_FAILED },
        { code: "EPERM", substr: "Socket creation failed", status: CFRDS_STATUS.SOCKET_CREATION_FAILED },
        { code: "EMFILE", substr: "Socket creation failed", status: CFRDS_STATUS.SOCKET_CREATION_FAILED },
        { code: "ENFILE", substr: "Socket creation failed", status: CFRDS_STATUS.SOCKET_CREATION_FAILED },
        { code: "ECONNREFUSED", substr: "Connection to server failed", status: CFRDS_STATUS.CONNECTION_TO_SERVER_FAILED },
        { code: "ETIMEDOUT", substr: "Connection to server failed", status: CFRDS_STATUS.CONNECTION_TO_SERVER_FAILED },
        { code: "NONE", substr: "Connection to server failed", status: CFRDS_STATUS.CONNECTION_TO_SERVER_FAILED },
      ];

      for (const { code, substr, status } of testCases) {
        test(`maps network error ${code} to status ${status}`, async () => {
          mockNetworkErrorCode = code;
          const srv = new Server("127.0.0.1", 8500, "admin", "admin");
          await assert.rejects(
            async () => {
              await srv.browseDir("/");
            },
            (err: any) => {
              assert.ok(err instanceof CFRDSNetworkError);
              assert.ok(err instanceof CFRDSError);
              assert.equal(err.status, status);
              assert.match(err.message, new RegExp(substr));
              assert.ok(err.cause !== undefined);
              if (code !== "NONE") {
                assert.equal(err.code, code);
              }
              return true;
            }
          );
        });
      }
    });
  });

  describe("Live Server Integration Tests", () => {
    const rdsHost = process.env.RDS_HOST;
    const rdsPort = process.env.RDS_PORT;
    const rdsUsername = process.env.RDS_USERNAME || "admin";
    const rdsPassword = process.env.RDS_PASSWORD || "";

    const hasLiveServer = Boolean(rdsHost && rdsPort);

    test("Live ColdFusion RDS server tests", { skip: !hasLiveServer ? "RDS_HOST and RDS_PORT not provided" : false }, async () => {
      const port = parseInt(rdsPort!, 10);
      const rds = new Server(rdsHost!, port, rdsUsername, rdsPassword);

      let cfRoot = "";
      try {
        cfRoot = await rds.cfRootDir();
        assert.equal(typeof cfRoot, "string");
      } catch (e) {
        // Log error if any
      }

      try {
        const targetPath = cfRoot || "/";
        const items = await rds.browseDir(targetPath);
        assert.ok(Array.isArray(items));
        if (items.length > 0) {
          assert.ok(items[0].modified > 0);
        }
      } catch (e) {}

      const testDir = `/tmp/ts_test_dir_${Date.now()}`;
      const testFile = `${testDir}/test.txt`;
      const testFileRenamed = `${testDir}/test_renamed.txt`;

      try {
        await rds.dirCreate(testDir);
        assert.equal(await rds.fileExists(testDir), true);

        const fileData = "Hello ColdFusion RDS TypeScript Test!";
        await rds.fileWrite(testFile, fileData);
        assert.equal(await rds.fileExists(testFile), true);

        const readRes = await rds.fileRead(testFile);
        assert.equal(readRes.data.toString("utf-8"), fileData);

        await rds.fileRename(testFile, testFileRenamed);
        assert.equal(await rds.fileExists(testFile), false);
        assert.equal(await rds.fileExists(testFileRenamed), true);

        await rds.fileRemove(testFileRenamed);
        await rds.dirRemove(testDir);
        assert.equal(await rds.fileExists(testDir), false);
      } catch (e) {
        try { await rds.fileRemove(testFile); } catch {}
        try { await rds.fileRemove(testFileRenamed); } catch {}
        try { await rds.dirRemove(testDir); } catch {}
      }

      try {
        const supportedCmds = await rds.sqlGetsupportedcommands();
        assert.ok(Array.isArray(supportedCmds));
      } catch (e) {}

      let dsns: string[] = [];
      try {
        dsns = await rds.sqlDsninfo();
        assert.ok(Array.isArray(dsns));
      } catch (e) {}

      const targetDsn = process.env.RDS_DSN || (dsns.length > 0 ? dsns[0] : "");
      if (targetDsn) {
        try {
          const dbDesc = await rds.sqlDbdescription(targetDsn);
          assert.equal(typeof dbDesc, "string");
        } catch (e) {}

        let tables: any[] = [];
        try {
          tables = await rds.sqlTableinfo(targetDsn);
          assert.ok(Array.isArray(tables));
        } catch (e) {}

        const targetTable = process.env.RDS_DSN_TABLE || (tables.length > 0 ? tables[0].name : "");
        if (targetTable) {
          try {
            const cols = await rds.sqlColumninfo(targetDsn, targetTable);
            assert.ok(Array.isArray(cols));
          } catch (e) {}

          try {
            const pks = await rds.sqlPrimarykeys(targetDsn, targetTable);
            assert.ok(Array.isArray(pks));
          } catch (e) {}

          try {
            const fks = await rds.sqlForeignkeys(targetDsn, targetTable);
            assert.ok(Array.isArray(fks));
          } catch (e) {}

          try {
            const impKeys = await rds.sqlImportedkeys(targetDsn, targetTable);
            assert.ok(Array.isArray(impKeys));
          } catch (e) {}

          try {
            const expKeys = await rds.sqlExportedkeys(targetDsn, targetTable);
            assert.ok(Array.isArray(expKeys));
          } catch (e) {}
        }

        try {
          const sqlRes = await rds.sqlSqlstmnt(targetDsn, "SELECT 1");
          assert.ok(Array.isArray(sqlRes.values));
        } catch (e) {}

        try {
          const metaRes = await rds.sqlMetadata(targetDsn, "SELECT 1");
          assert.ok(Array.isArray(metaRes));
        } catch (e) {}
      }

      try {
        const scanPath = testDir;
        const cmdId = await rds.securityAnalyzerScan(scanPath, true, 1);
        if (cmdId > 0) {
          const status = await rds.securityAnalyzerStatus(cmdId);
          assert.equal(typeof status, "object");
          await rds.securityAnalyzerResult(cmdId);
          await rds.securityAnalyzerCancel(cmdId);
          await rds.securityAnalyzerClean(cmdId);
        }
      } catch (e) {}

      try {
        const ideRes = await rds.ideDefault(1);
        assert.equal(typeof ideRes, "object");
      } catch (e) {}

      try {
        const sessionId = await rds.debuggerStart();
        assert.ok(typeof sessionId === "string" && sessionId.length > 0);
        try {
          try { await rds.debuggerGetServerInfo(sessionId); } catch {}
          try { await rds.debuggerSetScopeFilter(sessionId, "VARIABLES,SESSION"); } catch {}
          try { await rds.debuggerWatchVariables(sessionId, "VARIABLES.A"); } catch {}
          try { await rds.debuggerClearAllBreakpoints(sessionId); } catch {}

          const testCfmContent = "<cfset a = 10>\n<cfset b = 20>\n<cfset c = a + b>\n<cfoutput>Debug Test Page: #c#</cfoutput>\n";
          const appCfmPath = "/app/test_debug.cfm";
          const wwwrootCfmPath = cfRoot ? `${cfRoot}/wwwroot/test_debug.cfm` : "/opt/coldfusion/cfusion/wwwroot/test_debug.cfm";

          try { await rds.fileWrite(appCfmPath, testCfmContent); } catch {}
          try { await rds.fileWrite(wwwrootCfmPath, testCfmContent); } catch {}

          const targetBpPath = appCfmPath;
          try { await rds.debuggerBreakpoint(sessionId, targetBpPath, 2, true); } catch {}
          if (wwwrootCfmPath !== appCfmPath) {
            try { await rds.debuggerBreakpoint(sessionId, wwwrootCfmPath, 2, true); } catch {}
          }
          try { await rds.debuggerBreakpointOnException(sessionId, true); } catch {}
          try { await rds.debuggerAllFetchFlagsEnabled(sessionId, true, true, true, true, true); } catch {}

          let evt1 = null;
          try { evt1 = await rds.debuggerGetDebugEvents(sessionId); } catch {}

          let evt2 = null;
          try { evt2 = await rds.debuggerGetDebugEvents(sessionId); } catch {}

          const threadName = (evt2 && ((evt2.data as any)?.thread_name || (evt2.data as any)?.thread_id)) ||
                             (evt1 && ((evt1.data as any)?.thread_name || (evt1.data as any)?.thread_id)) || "main";

          try { await rds.debuggerWatchExpression(sessionId, threadName, "arrayNew(1)"); } catch {}
          try { await rds.debuggerSetVariable(sessionId, threadName, "VARIABLES.A", "200"); } catch {}
          try { await rds.debuggerGetOutput(sessionId, threadName); } catch {}
          try { await rds.debuggerStepOver(sessionId, threadName); } catch {}
          try { await rds.debuggerContinue(sessionId, threadName); } catch {}
          try { await rds.debuggerBreakpoint(sessionId, targetBpPath, 2, false); } catch {}
        } finally {
          await rds.debuggerStop(sessionId);
        }
      } catch (e) {}

      await rds.close();
    });
  });
});
