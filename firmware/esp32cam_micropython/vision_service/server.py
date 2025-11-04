"""Minimal HTTP service that uses OpenAI's vision models to describe images.

Run this on a laptop or server that the ESP32-CAM can reach over the network.
Expose the `/analyze` endpoint defined below. The MicroPython firmware sends
JPEG bytes to this endpoint and expects back JSON in the form
`{"description": "<text>"}`.
"""

import base64
import os
from typing import Annotated

from fastapi import FastAPI, Header, HTTPException, Request, Response
from fastapi.responses import JSONResponse
from openai import OpenAI


API_KEY_ENV = "OPENAI_API_KEY"


def build_client() -> OpenAI:
    api_key = os.environ.get(API_KEY_ENV)
    if not api_key:
        raise RuntimeError(
            f"Environment variable {API_KEY_ENV} is required to run the vision service."
        )
    return OpenAI(api_key=api_key)


client = build_client()
app = FastAPI(title="ESP32 Vision Relay")


@app.post("/analyze")
async def analyze(
    request: Request,
    x_api_key: Annotated[str | None, Header(convert_underscores=False)] = None,
) -> Response:
    expected_key = os.environ.get("VISION_SHARED_SECRET", "")
    if expected_key and expected_key != (x_api_key or ""):
        raise HTTPException(status_code=401, detail="Invalid API key")

    image_bytes = await request.body()
    if not image_bytes:
        raise HTTPException(status_code=400, detail="Missing image data")

    image_b64 = base64.b64encode(image_bytes).decode("ascii")

    try:
        response = client.responses.create(
            model="gpt-4o-mini",
            input=[
                {
                    "role": "user",
                    "content": [
                        {"type": "input_text", "text": "Descreva esta imagem em português."},
                        {"type": "input_image", "image_base64": image_b64},
                    ],
                }
            ],
            max_output_tokens=256,
        )
    except Exception as exc:
        raise HTTPException(status_code=502, detail=f"Vision provider error: {exc}")

    try:
        message = response.output[0].content[0].text.strip()  # type: ignore[attr-defined]
    except Exception:
        raise HTTPException(status_code=500, detail="Malformed response from vision model")

    return JSONResponse({"description": message})


@app.get("/healthz")
async def healthz() -> Response:
    return JSONResponse({"status": "ok"})
