import { CFRDS_STATUS, CFRDSError, ServerContext } from "./types";

const FILLUP_KEY = Buffer.from("4p0L@r1$", "utf-8");
const HEX_CHARS = "0123456789abcdef";

export function encodePassword(password: string): string {
  if (!password) return "";
  const pwdBytes = Buffer.from(password, "utf-8");
  const out: string[] = [];
  for (let i = 0; i < pwdBytes.length; i++) {
    const enc = pwdBytes[i] ^ FILLUP_KEY[i % FILLUP_KEY.length];
    out.push(HEX_CHARS[(enc & 0xf0) >> 4]);
    out.push(HEX_CHARS[enc & 0x0f]);
  }
  return out.join("");
}

export function parseNumber(data: Buffer, offset: number): [number, number] {
  const colonPos = data.indexOf(0x3a, offset);
  if (colonPos === -1) {
    throw new CFRDSError(CFRDS_STATUS.RESPONSE_ERROR, "Failed to parse number: missing ':' delimiter");
  }
  const str = data.toString("utf-8", offset, colonPos);
  const val = parseInt(str, 10);
  if (isNaN(val)) {
    throw new CFRDSError(CFRDS_STATUS.RESPONSE_ERROR, "Failed to parse number: non-integer value");
  }
  return [val, colonPos + 1];
}

export function parseString(data: Buffer, offset: number): [string, number] {
  const [size, newOffset] = parseNumber(data, offset);
  if (size < 0 || newOffset + size > data.length) {
    throw new CFRDSError(CFRDS_STATUS.RESPONSE_ERROR, "Failed to parse string: bounds error");
  }
  return [data.toString("utf-8", newOffset, newOffset + size), newOffset + size];
}

export function parseBytearray(data: Buffer, offset: number): [Buffer, number] {
  const [size, newOffset] = parseNumber(data, offset);
  if (size < 0 || newOffset + size > data.length) {
    throw new CFRDSError(CFRDS_STATUS.RESPONSE_ERROR, "Failed to parse bytearray: bounds error");
  }
  return [data.subarray(newOffset, newOffset + size), newOffset + size];
}

export function parseStringListItem(s: string): string[] {
  const items: string[] = [];
  let i = 0;
  const n = s.length;
  while (i < n) {
    if (s[i] === '"') {
      const endQ = s.indexOf('"', i + 1);
      if (endQ === -1) {
        items.push(s.slice(i + 1));
        break;
      }
      items.push(s.slice(i + 1, endQ));
      i = endQ + 1;
      if (i < n && s[i] === ",") {
        i++;
      }
    } else {
      const commaPos = s.indexOf(",", i);
      if (commaPos === -1) {
        items.push(s.slice(i));
        break;
      }
      items.push(s.slice(i, commaPos));
      i = commaPos + 1;
    }
  }
  return items;
}

export function buildPayload(items: (string | Buffer)[]): Buffer {
  const totalCnt = items.length;
  const parts: Buffer[] = [];

  parts.push(Buffer.from(`${totalCnt}:`, "utf-8"));

  for (const item of items) {
    const buf = typeof item === "string" ? Buffer.from(item, "utf-8") : item;
    parts.push(Buffer.from(`STR:${buf.length}:`, "utf-8"));
    parts.push(buf);
  }

  return Buffer.concat(parts);
}
