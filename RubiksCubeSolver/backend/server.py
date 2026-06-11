import json
import os
import platform
import shutil
import subprocess
from pathlib import Path

from flask import Flask, jsonify, request, send_from_directory


BASE_DIR = Path(__file__).resolve().parent
PROJECT_DIR = BASE_DIR.parent
FRONTEND_DIR = PROJECT_DIR / "frontend"
SOLVER_EXE = BASE_DIR / ("solver.exe" if platform.system().lower().startswith("win") else "solver")

app = Flask(__name__, static_folder=str(FRONTEND_DIR), static_url_path="")


def source_files():
    files = [BASE_DIR / "solver.cpp"]
    files.extend(sorted((BASE_DIR / "algorithms").glob("*.cpp")))
    return files


def find_compiler():
    compiler = shutil.which("g++")
    if compiler:
        return compiler

    windows_candidates = [
        Path("C:/MinGW/bin/g++.exe"),
        Path("C:/msys64/ucrt64/bin/g++.exe"),
        Path("C:/msys64/mingw64/bin/g++.exe"),
    ]
    for candidate in windows_candidates:
        if candidate.exists():
            return str(candidate)
    return None


def needs_rebuild():
    if not SOLVER_EXE.exists():
        return True
    exe_time = SOLVER_EXE.stat().st_mtime
    return any(path.stat().st_mtime > exe_time for path in source_files())


def compile_solver():
    compiler = find_compiler()
    if not compiler:
        return False, "g++ was not found. Install MinGW-w64 and add its bin folder to PATH."

    command = [
        compiler,
        "-std=c++17",
        "-O2",
        "-Wall",
        "-Wextra",
        *[str(path) for path in source_files()],
        "-o",
        str(SOLVER_EXE),
    ]

    completed = subprocess.run(
        command,
        cwd=BASE_DIR,
        text=True,
        capture_output=True,
        timeout=60,
    )

    if completed.returncode != 0:
        return False, completed.stderr or completed.stdout or "C++ compilation failed."
    return True, "C++ solver compiled successfully."


@app.after_request
def add_cors_headers(response):
    response.headers["Access-Control-Allow-Origin"] = "*"
    response.headers["Access-Control-Allow-Headers"] = "Content-Type"
    response.headers["Access-Control-Allow-Methods"] = "GET,POST,OPTIONS"
    return response


@app.route("/")
def home():
    return send_from_directory(FRONTEND_DIR, "index.html")


@app.route("/<path:path>")
def static_files(path):
    return send_from_directory(FRONTEND_DIR, path)


@app.route("/api/health")
def health():
    compiler = find_compiler()
    return jsonify(
        {
            "status": "ok",
            "compiler": compiler,
            "solverExists": SOLVER_EXE.exists(),
            "project": "Rubik's Cube Solver ADA Mini Project",
        }
    )


@app.route("/api/compile", methods=["POST"])
def compile_endpoint():
    ok, message = compile_solver()
    return jsonify({"success": ok, "message": message}), 200 if ok else 500


@app.route("/api/solve", methods=["POST", "OPTIONS"])
def solve():
    if request.method == "OPTIONS":
        return ("", 204)

    payload = request.get_json(force=True, silent=True) or {}
    if "state" not in payload:
        return jsonify({"valid": False, "solved": False, "error": "Missing cube state."}), 400

    if needs_rebuild():
        ok, message = compile_solver()
        if not ok:
            return jsonify({"valid": False, "solved": False, "error": message}), 500

    try:
        completed = subprocess.run(
            [str(SOLVER_EXE)],
            input=json.dumps(payload),
            cwd=BASE_DIR,
            text=True,
            capture_output=True,
            timeout=25,
        )
    except subprocess.TimeoutExpired:
        return jsonify(
            {
                "valid": True,
                "solved": False,
                "error": "Solver timed out. Try A*, IDDFS, or reduce the scramble depth.",
            }
        ), 504

    if completed.returncode != 0:
        return jsonify(
            {
                "valid": False,
                "solved": False,
                "error": completed.stderr or "Solver execution failed.",
            }
        ), 500

    try:
        return jsonify(json.loads(completed.stdout))
    except json.JSONDecodeError:
        return jsonify(
            {
                "valid": False,
                "solved": False,
                "error": "Solver returned invalid JSON.",
                "raw": completed.stdout,
                "stderr": completed.stderr,
            }
        ), 500


if __name__ == "__main__":
    print("Rubik's Cube Solver is running at http://127.0.0.1:5000")
    app.run(host="127.0.0.1", port=5000, debug=True)
