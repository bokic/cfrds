import * as http from "http";
import {
  CFRDS_STATUS,
  CFRDSError,
  CFRDSNetworkError,
  CFRDSResponseError,
  CFRDSCommandError,
  ServerContext,
} from "./types";
import { buildPayload, parseNumber } from "./parser";

const MAX_RESPONSE_SIZE = 100 * 1024 * 1024;

export function sendRdsCommand(
  ctx: ServerContext,
  command: string,
  args: (string | Buffer)[]
): Promise<Buffer> {
  const allItems = [...args];
  if (ctx.config.username !== undefined && ctx.config.username.length > 0) {
    allItems.push(ctx.config.username);
  }
  if (ctx.config.password !== undefined && ctx.config.password.length > 0) {
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
    Connection: "keep-alive",
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
        agent: ctx.agent,
      },
      (res) => {
        const chunks: Buffer[] = [];
        let totalSize = 0;

        res.on("data", (chunk: Buffer) => {
          totalSize += chunk.length;
          if (totalSize > MAX_RESPONSE_SIZE) {
            res.destroy();
            const err = new CFRDSResponseError(
              `Response too large: ${totalSize} bytes`,
              CFRDS_STATUS.RESPONSE_TOO_LARGE
            );
            reject(err);
            return;
          }
          chunks.push(chunk);
        });

        res.on("end", () => {
          if (res.statusCode !== 200) {
            const msg = `HTTP ${res.statusCode} ${res.statusMessage || ""}`.trim();
            reject(
              new CFRDSResponseError(
                `HTTP_RESPONSE_NOT_FOUND: ${msg}`,
                CFRDS_STATUS.HTTP_RESPONSE_NOT_FOUND
              )
            );
            return;
          }

          const body = Buffer.concat(chunks);

          try {
            const [errCode, offset] = parseNumber(body, 0);

            if (errCode < 0) {
              const errMsg = body.toString("utf-8", offset);
              reject(
                new CFRDSCommandError(
                  `COMMAND_FAILED: ${errMsg}`,
                  CFRDS_STATUS.COMMAND_FAILED
                )
              );
              return;
            }

            resolve(body);
          } catch (e) {
            const msg = e instanceof Error ? e.message : String(e);
            if (e instanceof CFRDSError) {
              reject(e);
            } else {
              reject(
                new CFRDSResponseError(`RESPONSE_ERROR: ${msg}`, {
                  status: CFRDS_STATUS.RESPONSE_ERROR,
                  cause: e,
                })
              );
            }
          }
        });

        res.on("error", (err: Error) => {
          reject(
            new CFRDSNetworkError(
              `Reading from socket failed: ${err.message}`,
              {
                status: CFRDS_STATUS.READING_FROM_SOCKET_FAILED,
                cause: err,
              }
            )
          );
        });
      }
    );

    req.on("timeout", () => {
      req.destroy();
      const msg = "Connection timed out";
      reject(
        new CFRDSNetworkError(msg, {
          status: CFRDS_STATUS.CONNECTION_TO_SERVER_FAILED,
          code: "ETIMEDOUT",
        })
      );
    });

    req.on("error", (err: any) => {
      let desc = "Connection to server failed";
      let status = CFRDS_STATUS.CONNECTION_TO_SERVER_FAILED;

      if (err && (err.code === "ENOTFOUND" || err.code === "EAI_AGAIN")) {
        desc = "Socket host not found";
        status = CFRDS_STATUS.SOCKET_HOST_NOT_FOUND;
      } else if (
        err &&
        (err.code === "EADDRNOTAVAIL" ||
          err.code === "EACCES" ||
          err.code === "EPERM" ||
          err.code === "EMFILE" ||
          err.code === "ENFILE")
      ) {
        desc = "Socket creation failed";
        status = CFRDS_STATUS.SOCKET_CREATION_FAILED;
      }

      const msg = `${desc}: ${err.message}`;
      reject(
        new CFRDSNetworkError(msg, {
          status,
          code: err?.code,
          cause: err,
        })
      );
    });

    req.write(payload);
    req.end();
  });
}
