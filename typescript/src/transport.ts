import * as http from "http";
import { CFRDS_STATUS, CFRDSError, ServerContext } from "./types";
import { buildPayload, parseNumber } from "./parser";

const MAX_RESPONSE_SIZE = 100 * 1024 * 1024;

export function sendRdsCommand(
  ctx: ServerContext,
  command: string,
  args: (string | Buffer)[]
): Promise<Buffer> {
  ctx.errorCode = 1;
  ctx.error = null;

  const allItems = [...args];
  if (ctx.config.username !== undefined) {
    allItems.push(ctx.config.username);
  }
  if (ctx.config.password !== undefined) {
    allItems.push(ctx.encodedPassword);
  }

  const payload = buildPayload(allItems);

  const path = `/CFIDE/main/ide.cfm?CFSRV=IDE&ACTION=${command}`;
  const headers: Record<string, string> = {
    "User-Agent": "Mozilla/3.0 (compatible; Macromedia RDS Client)",
    Accept: "text/html, */*",
    "Accept-Encoding": "deflate",
    "Content-Type": "text/html",
    "Content-Length": payload.length.toString(),
    Connection: "close",
  };

  return new Promise<Buffer>((resolve, reject) => {
    const req = http.request(
      {
        hostname: ctx.config.host,
        port: ctx.config.port,
        path,
        method: "POST",
        headers,
        timeout: 30000,
      },
      (res) => {
        const chunks: Buffer[] = [];
        let totalSize = 0;

        res.on("data", (chunk: Buffer) => {
          totalSize += chunk.length;
          if (totalSize > MAX_RESPONSE_SIZE) {
            res.destroy();
            const err = new CFRDSError(
              CFRDS_STATUS.RESPONSE_TOO_LARGE,
              `Response too large: ${totalSize} bytes`
            );
            ctx.errorCode = CFRDS_STATUS.RESPONSE_TOO_LARGE;
            ctx.error = err.message;
            reject(err);
            return;
          }
          chunks.push(chunk);
        });

        res.on("end", () => {
          if (res.statusCode !== 200) {
            const msg = `HTTP ${res.statusCode} ${res.statusMessage || ""}`;
            ctx.errorCode = CFRDS_STATUS.HTTP_RESPONSE_NOT_FOUND;
            ctx.error = msg;
            reject(new CFRDSError(CFRDS_STATUS.HTTP_RESPONSE_NOT_FOUND, msg));
            return;
          }

          const body = Buffer.concat(chunks);

          try {
            const [errCode, offset] = parseNumber(body, 0);
            ctx.errorCode = errCode;

            if (errCode < 0) {
              const errMsg = body.toString("utf-8", offset);
              ctx.errorCode = CFRDS_STATUS.COMMAND_FAILED;
              ctx.error = errMsg;
              reject(new CFRDSError(CFRDS_STATUS.COMMAND_FAILED, errMsg));
              return;
            }

            resolve(body.subarray(offset));
          } catch (e) {
            if (e instanceof CFRDSError) {
              ctx.errorCode = e.status;
              ctx.error = e.message;
              reject(e);
            } else {
              const msg = e instanceof Error ? e.message : String(e);
              ctx.errorCode = CFRDS_STATUS.RESPONSE_ERROR;
              ctx.error = msg;
              reject(new CFRDSError(CFRDS_STATUS.RESPONSE_ERROR, msg));
            }
          }
        });

        res.on("error", (err) => {
          ctx.errorCode = CFRDS_STATUS.READING_FROM_SOCKET_FAILED;
          ctx.error = err.message;
          reject(new CFRDSError(CFRDS_STATUS.READING_FROM_SOCKET_FAILED, err.message));
        });
      }
    );

    req.on("timeout", () => {
      req.destroy();
      ctx.errorCode = CFRDS_STATUS.CONNECTION_TO_SERVER_FAILED;
      ctx.error = "Connection timed out";
      reject(new CFRDSError(CFRDS_STATUS.CONNECTION_TO_SERVER_FAILED, "Connection timed out"));
    });

    req.on("error", (err) => {
      ctx.errorCode = CFRDS_STATUS.CONNECTION_TO_SERVER_FAILED;
      ctx.error = err.message;
      reject(new CFRDSError(CFRDS_STATUS.CONNECTION_TO_SERVER_FAILED, err.message));
    });

    req.write(payload);
    req.end();
  });
}
