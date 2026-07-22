import * as http from "http";
import { CFRDSError, ServerContext } from "./types";
import { buildPayload, parseNumber } from "./parser";

const MAX_RESPONSE_SIZE = 100 * 1024 * 1024;

export function sendRdsCommand(
  ctx: ServerContext,
  command: string,
  args: (string | Buffer)[]
): Promise<Buffer> {
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
            const err = new CFRDSError(`Response too large: ${totalSize} bytes`);
            ctx.error = err.message;
            reject(err);
            return;
          }
          chunks.push(chunk);
        });

        res.on("end", () => {
          if (res.statusCode !== 200) {
            const msg = `HTTP ${res.statusCode} ${res.statusMessage || ""}`;
            ctx.error = msg;
            reject(new CFRDSError(`HTTP_RESPONSE_NOT_FOUND: ${msg}`));
            return;
          }

          const body = Buffer.concat(chunks);

          try {
            const [errCode, offset] = parseNumber(body, 0);

            if (errCode < 0) {
              const errMsg = body.toString("utf-8", offset);
              ctx.error = errMsg;
              reject(new CFRDSError(`COMMAND_FAILED: ${errMsg}`));
              return;
            }

            resolve(body);
          } catch (e) {
            const msg = e instanceof Error ? e.message : String(e);
            ctx.error = msg;
            reject(e instanceof CFRDSError ? e : new CFRDSError(`RESPONSE_ERROR: ${msg}`));
          }
        });

        res.on("error", (err) => {
          ctx.error = err.message;
          reject(new CFRDSError(`Reading from socket failed: ${err.message}`));
        });
      }
    );

    req.on("timeout", () => {
      req.destroy();
      ctx.error = "Connection timed out";
      reject(new CFRDSError("Connection timed out"));
    });

    req.on("error", (err: any) => {
      let desc = "Connection to server failed";
      if (err && (err.code === "ENOTFOUND" || err.code === "EAI_AGAIN")) {
        desc = "Socket host not found";
      } else if (
        err &&
        (err.code === "EADDRNOTAVAIL" ||
          err.code === "EACCES" ||
          err.code === "EPERM" ||
          err.code === "EMFILE" ||
          err.code === "ENFILE")
      ) {
        desc = "Socket creation failed";
      }
      ctx.error = `${desc}: ${err.message}`;
      reject(new CFRDSError(`${desc}: ${err.message}`));
    });

    req.write(payload);
    req.end();
  });
}
