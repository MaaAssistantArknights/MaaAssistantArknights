import os
from openai import OpenAI

SYSTEM_PROMPT = """You are a C/C++ code repair assistant for a large repo.
- Read the tool evidence and propose a SMALL unified diff patch (git apply compatible).
- Constraints: minimal and safe changes; avoid broad refactors; keep behavior unless fixing bug.
- Only touch necessary files; add null checks or bounds checks when appropriate; add comments if needed.
- Output ONLY the unified diff, nothing else."""

USER_PROMPT_TMPL = """Repository is a large C++ project.
Evidence from tools:
{evidence}

Constraints:
- Max changed lines: {max_lines}
- Only modify necessary files.
- Ensure code compiles.

Generate a patch as unified diff:"""

def call_llm(model: str, api_key: str, system_prompt: str, user_prompt: str) -> str:
    client = OpenAI(api_key=api_key)
    resp = client.chat.completions.create(
        model=model,
        messages=[
            {"role": "system", "content": system_prompt},
            {"role": "user", "content": user_prompt}
        ],
        temperature=0.1,
    )
    return resp.choices[0].message.content or ""

def propose_patch(evidence: str, repo_path: str, llm_cfg: dict) -> str:
    api_key = os.getenv(llm_cfg["api_key_env"], "")
    if not api_key:
        raise RuntimeError(f"Missing API key env: {llm_cfg['api_key_env']}")
    prompt = USER_PROMPT_TMPL.format(evidence=evidence, max_lines=llm_cfg.get("max_changed_lines", 120))
    patch = call_llm(llm_cfg["model"], api_key, SYSTEM_PROMPT, prompt)
    return patch.strip()
