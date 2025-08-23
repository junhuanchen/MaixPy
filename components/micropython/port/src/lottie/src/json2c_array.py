#!/usr/bin/env python3
# json_to_c_literal.py
import json
import sys

def to_c_literal(j: str) -> str:
    """把任意字符串转义成可直接塞进 C 字符串字面量的形式"""
    # 先按 C 规则转义 \ 和 "
    escaped = (
        j.replace("\\", "\\\\")
          .replace('"', '\\"')
          .replace("\n", "")        # 去掉换行，保持单行
    )
    return f'"{escaped}"'

if __name__ == "__main__":
    if len(sys.argv) == 1:
        # 从 stdin 读 JSON
        raw_json = sys.stdin.read()
    else:
        # 也可以直接给文件路径
        with open(sys.argv[1], encoding="utf-8") as f:
            raw_json = f.read()

    # 如果输入是 JSON 对象/数组，先转字符串
    try:
        parsed = json.loads(raw_json)
        raw_json = json.dumps(parsed, separators=(",", ":"))  # 去掉多余空格
    except ValueError:
        pass  # 不是 JSON，直接按纯文本处理

    print(to_c_literal(raw_json))
    