import io
import os
from typing import List

from fastapi import FastAPI, File, UploadFile, HTTPException
from fastapi.responses import JSONResponse
from PIL import Image
from torch.nn.modules.container import Sequential
from torch.serialization import add_safe_globals
from ultralytics import YOLO
from ultralytics.nn.tasks import DetectionModel

# Permite carregar pesos YOLO quando torch.load usa weights_only=True (PyTorch >=2.6)
os.environ.setdefault("TORCH_LOAD_WEIGHTS_ONLY", "0")
add_safe_globals([DetectionModel, Sequential])

MODEL_PATH = os.getenv("YOLO_MODEL_PATH", "yolov8n.pt")
CONFIDENCE_THRESHOLD = float(os.getenv("CONFIDENCE_THRESHOLD", "0.35"))

app = FastAPI(title="ESP32-CAM Inference Service")
model = YOLO(MODEL_PATH)


@app.post("/infer")
async def infer(image: UploadFile = File(...)) -> JSONResponse:
    if image.content_type not in ("image/jpeg", "image/jpg"):
        raise HTTPException(status_code=400, detail="Use JPEG (content-type image/jpeg)")

    raw_bytes = await image.read()
    pil_image = Image.open(io.BytesIO(raw_bytes)).convert("RGB")
    results = model.predict(pil_image, verbose=False)[0]

    objects: List[dict] = []
    for box in results.boxes:
        confidence = float(box.conf)
        if confidence < CONFIDENCE_THRESHOLD:
            continue
        cls_id = int(box.cls)
        label = model.names.get(cls_id, f"class_{cls_id}")
        objects.append(
            {
                "label": label,
                "confidence": confidence,
            }
        )

    return JSONResponse({"objects": objects})


if __name__ == "__main__":
    import uvicorn

    uvicorn.run(
        "inference_server:app",
        host="0.0.0.0",
        port=int(os.getenv("PORT", "8000")),
        reload=False,
    )
