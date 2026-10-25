"""Run one real MCP request and retain its text/image evidence."""
import asyncio
import base64
import json
import os
from pathlib import Path
import sys

from mcp import ClientSession, StdioServerParameters
from mcp.client.stdio import stdio_client


async def main():
    request_file = Path(sys.argv[1])
    request = json.loads(request_file.read_text(encoding="utf-8-sig"))
    environment = dict(os.environ)
    environment["UNREAL_PROJECT_DIR"] = "C:/Users/tarik/Documents/GitHub/AGP-Content/Unreal/P5G4_KnightHeart"
    parameters = StdioServerParameters(
        command="C:/Users/tarik/Documents/UnrealMCP/.venv/Scripts/python.exe",
        args=["-m", "unreal_mcp"], env=environment,
    )
    async with stdio_client(parameters) as (reader, writer):
        async with ClientSession(reader, writer) as session:
            await session.initialize()
            result = await session.call_tool(request["tool"], request.get("arguments", {}))
            record = result.model_dump(mode="json")
            for index, content in enumerate(record.get("content", [])):
                if content["type"] == "image":
                    path = request_file.with_name(request_file.stem + f"-image-{index}.png")
                    path.write_bytes(base64.b64decode(content.pop("data")))
                    content["saved_path"] = str(path.resolve())
            request_file.with_suffix(".result.json").write_text(json.dumps(record, indent=2), encoding="utf-8")
            print(json.dumps(record, indent=2))
            if result.isError:
                raise SystemExit(1)
            for content in result.content:
                if content.type == "text":
                    try:
                        payload = json.loads(content.text)
                    except (ValueError, TypeError):
                        continue
                    if isinstance(payload, dict) and payload.get("success") is False:
                        raise SystemExit(1)


asyncio.run(main())
