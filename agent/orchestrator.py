import os
from typing import Optional
from openai import OpenAI

def call_llm(cfg, prompt: str, sys_hint: Optional[str] = None) -> str:
    if cfg["llm"]["provider"] != "openai":
        raise NotImplementedError("示范OpenAI，其他按需扩展。")
    client = OpenAI(api_key=os.getenv(cfg["llm"]["api_key_env"]))
    messages = []
    if sys_hint:
        messages.append({"role":"system","content":sys_hint})
    messages.append({"role":"user","content":prompt})
    resp = client.chat.completions.create(
        model=cfg["llm"]["model"],
        messages=messages,
        temperature=0.2
    )
    return resp.choices[0].message.content.strip()
