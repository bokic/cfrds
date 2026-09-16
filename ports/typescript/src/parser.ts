import { XMLParser } from "fast-xml-parser";
import { CFRDS_STATUS, CFRDSError, CFRDSResponseError, ServerContext } from "./types";

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
    throw new CFRDSResponseError(
      "Failed to parse number: missing ':' delimiter",
      CFRDS_STATUS.RESPONSE_ERROR
    );
  }
  const str = data.toString("utf-8", offset, colonPos);
  const val = parseInt(str, 10);
  if (isNaN(val)) {
    throw new CFRDSResponseError(
      "Failed to parse number: non-integer value",
      CFRDS_STATUS.RESPONSE_ERROR
    );
  }
  return [val, colonPos + 1];
}

export function parseString(data: Buffer, offset: number): [string, number] {
  const [size, newOffset] = parseNumber(data, offset);
  if (size < 0 || newOffset + size > data.length) {
    throw new CFRDSResponseError(
      "Failed to parse string: bounds error",
      CFRDS_STATUS.RESPONSE_ERROR
    );
  }
  return [data.toString("utf-8", newOffset, newOffset + size), newOffset + size];
}

export function parseBytearray(data: Buffer, offset: number): [Buffer, number] {
  const [size, newOffset] = parseNumber(data, offset);
  if (size < 0 || newOffset + size > data.length) {
    throw new CFRDSResponseError(
      "Failed to parse bytearray: bounds error",
      CFRDS_STATUS.RESPONSE_ERROR
    );
  }
  return [data.subarray(newOffset, newOffset + size), newOffset + size];
}

export function parseTimestamp(str: string): number {
  if (!str) return 0;
  // Format 1: ColdFusion Windows ticks format "num1,num2"
  if (str.includes(",")) {
    const parts = str.split(",");
    const num1 = parseInt(parts[0], 10);
    const num2 = parseInt(parts[1], 10);
    if (!isNaN(num1) && !isNaN(num2)) {
      return Math.floor((num1 + (num2 * 0x100000000)) / 10000) - 11644473600000;
    }
  }
  // Format 2: CF SimpleDateFormat format "hh:mm:ssa MM/dd/yyyy"
  const m = str.trim().match(/^(\d{1,2}):(\d{2}):(\d{2})([AaPp][Mm])\s+(\d{1,2})\/(\d{1,2})\/(\d{4})$/);
  if (m) {
    const [, h, min, s, ampm, month, day, year] = m;
    let hour = parseInt(h, 10);
    if (ampm.toUpperCase() === "PM" && hour < 12) hour += 12;
    if (ampm.toUpperCase() === "AM" && hour === 12) hour = 0;
    const d = new Date(parseInt(year, 10), parseInt(month, 10) - 1, parseInt(day, 10), hour, parseInt(min, 10), parseInt(s, 10));
    const t = d.getTime();
    if (!isNaN(t)) return t;
  }
  // Format 3: ISO 8601 or standard Date string format (e.g. "2026-07-22 05:00:00")
  const parsed = Date.parse(str.replace(" ", "T"));
  if (!isNaN(parsed)) return parsed;
  const direct = Date.parse(str);
  if (!isNaN(direct)) return direct;

  const num = parseInt(str, 10);
  return isNaN(num) ? 0 : num;
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

export interface XmlNode {
  tag: string;
  attrs: Record<string, string>;
  children: XmlNode[];
  text: string;
}

const xmlParser = new XMLParser({
  ignoreAttributes: false,
  attributeNamePrefix: "@_",
  preserveOrder: true,
  cdataPropName: "__cdata",
  trimValues: false,
  processEntities: true,
  htmlEntities: true,
});

export function parseXml(xml: string): XmlNode {
  const root: XmlNode = { tag: "?root?", attrs: {}, children: [], text: "" };
  if (!xml || !xml.trim()) {
    return root;
  }

  const ordered = xmlParser.parse(xml);
  if (!Array.isArray(ordered)) {
    return root;
  }

  function processList(items: any[], parent: XmlNode) {
    for (const item of items) {
      if (!item || typeof item !== "object") continue;
      const keys = Object.keys(item);
      for (const k of keys) {
        if (k === ":@") continue;
        if (k === "#text") {
          parent.text += String(item[k]);
        } else if (k === "__cdata") {
          const cdataArr = item[k];
          if (Array.isArray(cdataArr)) {
            for (const c of cdataArr) {
              if (c && c["#text"] !== undefined) {
                parent.text += String(c["#text"]);
              }
            }
          }
        } else {
          let tagName = k.toLowerCase();
          if (tagName.includes(":")) {
            tagName = tagName.split(":")[1];
          }
          const attrs: Record<string, string> = {};
          if (item[":@"]) {
            for (const [ak, av] of Object.entries(item[":@"])) {
              const cleanKey = ak.startsWith("@_") ? ak.slice(2).toLowerCase() : ak.toLowerCase();
              attrs[cleanKey] = String(av);
            }
          }
          const node: XmlNode = { tag: tagName, attrs, children: [], text: "" };
          parent.children.push(node);
          if (Array.isArray(item[k])) {
            processList(item[k], node);
          }
        }
      }
    }
  }

  processList(ordered, root);
  return root;
}

export function parseWddxNode(node: XmlNode): any {
  const tag = node.tag;
  if (tag === "null") {
    return null;
  } else if (tag === "boolean") {
    return node.attrs.value === "true";
  } else if (tag === "number") {
    const numStr = node.text.trim();
    const val = parseFloat(numStr);
    return isNaN(val) ? numStr : val;
  } else if (tag === "string") {
    return node.text;
  } else if (tag === "array") {
    return node.children.map(parseWddxNode);
  } else if (tag === "struct") {
    const obj: Record<string, any> = {};
    for (const child of node.children) {
      if (child.tag === "var") {
        const name = child.attrs.name;
        if (name) {
          obj[name] = child.children.length > 0 ? parseWddxNode(child.children[0]) : null;
        }
      }
    }
    return obj;
  }
  if (node.children.length > 0) {
    return parseWddxNode(node.children[0]);
  }
  return node.text || null;
}

export function wddxDeserialize(xml: string): any {
  if (!xml || !xml.trim()) return null;
  try {
    const root = parseXml(xml);
    const findData = (node: XmlNode): XmlNode | null => {
      if (node.tag === "data") return node;
      for (const child of node.children) {
        const res = findData(child);
        if (res) return res;
      }
      return null;
    };
    const dataNode = findData(root);
    if (dataNode && dataNode.children.length > 0) {
      return parseWddxNode(dataNode.children[0]);
    }
    if (root.children.length > 0) {
      return parseWddxNode(root.children[0]);
    }
    return null;
  } catch {
    return null;
  }
}

export function safeInt(s: string | null | undefined): number {
  if (!s) return 0;
  return /^-?\d+$/.test(s) ? parseInt(s, 10) : 0;
}
