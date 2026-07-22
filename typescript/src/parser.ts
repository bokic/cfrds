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

export interface XmlNode {
  tag: string;
  attrs: Record<string, string>;
  children: XmlNode[];
  text: string;
}

function unescapeXml(val: string): string {
  return val
    .replace(/&lt;/g, "<")
    .replace(/&gt;/g, ">")
    .replace(/&amp;/g, "&")
    .replace(/&quot;/g, '"')
    .replace(/&apos;/g, "'");
}

export function parseXml(xml: string): XmlNode {
  xml = xml.replace(/<!--[\s\S]*?-->/g, "");
  xml = xml.replace(/<\?[\s\S]*?\?>/g, "");

  const tokenRegex = /<!\[CDATA\[([\s\S]*?)\]\]>|<(\/)?([a-zA-Z0-9_\-:]+)((?:\s+[a-zA-Z0-9_\-:]+=(?:'[^']*'|"[^"]*"|[^\s>]+))*)\s*(\/)?>/g;

  let lastIndex = 0;
  const root: XmlNode = { tag: "?root?", attrs: {}, children: [], text: "" };
  const stack: XmlNode[] = [root];

  let match;
  while ((match = tokenRegex.exec(xml)) !== null) {
    const textBetween = xml.slice(lastIndex, match.index);
    if (textBetween) {
      const parent = stack[stack.length - 1];
      if (parent) {
        parent.text += unescapeXml(textBetween);
      }
    }

    const [full, cdataText, isClose, tagName, attrStr, isSelfClose] = match;

    if (cdataText !== undefined) {
      const parent = stack[stack.length - 1];
      if (parent) {
        parent.text += cdataText;
      }
    } else {
      let tagLower = tagName.toLowerCase();
      if (tagLower.includes(":")) {
        tagLower = tagLower.split(":")[1];
      }
      if (isClose) {
        if (stack.length > 1 && stack[stack.length - 1].tag === tagLower) {
          stack.pop();
        }
      } else {
        const attrs: Record<string, string> = {};
        if (attrStr) {
          const attrRegex = /([a-zA-Z0-9_\-:]+)=(?:'([^']*)'|"([^"]*)"|([^\s>]+))/g;
          let attrMatch;
          while ((attrMatch = attrRegex.exec(attrStr)) !== null) {
            attrs[attrMatch[1].toLowerCase()] = attrMatch[2] !== undefined ? attrMatch[2] : (attrMatch[3] !== undefined ? attrMatch[3] : attrMatch[4]);
          }
        }

        const node: XmlNode = {
          tag: tagLower,
          attrs,
          children: [],
          text: ""
        };

        const parent = stack[stack.length - 1];
        if (parent) {
          parent.children.push(node);
        }

        if (!isSelfClose) {
          stack.push(node);
        }
      }
    }
    lastIndex = tokenRegex.lastIndex;
  }

  const textEnd = xml.slice(lastIndex);
  if (textEnd) {
    const parent = stack[stack.length - 1];
    if (parent) {
      parent.text += unescapeXml(textEnd);
    }
  }

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

