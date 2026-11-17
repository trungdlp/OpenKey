"""Run instrumented tests and enforce >80% line coverage for every production file."""
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys


def llvm_tool(name):
    executable = shutil.which(name)
    if executable:
        return executable
    return subprocess.check_output(["xcrun", "--find", name], text=True).strip()


def main():
    build = Path(sys.argv[1]).absolute()
    output = build / "coverage"
    output.mkdir(exist_ok=True)
    for profile in output.glob("*.profraw"):
        profile.unlink()
    environment = dict(os.environ, LLVM_PROFILE_FILE=str(output / "%m-%p.profraw"))
    subprocess.run(["ctest", "--test-dir", str(build), "--output-on-failure"], env=environment, check=True)
    profile = output / "coverage.profdata"
    subprocess.run([llvm_tool("llvm-profdata"), "merge", "-sparse",
                    *map(str, output.glob("*.profraw")), "-o", str(profile)], check=True)
    suffix = ".exe" if os.name == "nt" else ""
    objects = [build / (name + suffix) for name in ("engine_tests", "sender_tests")]
    if os.name == "nt":
        objects.append(build / "update_installer_tests.exe")
    root = Path(__file__).resolve().parent.parent
    engine = root / "Sources/OpenKey/engine"
    sources = [engine / name for name in ("Engine.cpp", "Vietnamese.cpp", "Macro.cpp",
               "SmartSwitchKey.cpp", "ConvertTool.cpp", "CharacterOutput.h")]
    sources.append(build / "Senders.inc")
    if os.name == "nt":
        sources.append(root / "Sources/OpenKey/win32/OpenKey/OpenKeyUpdate/UpdateInstaller.h")
    arguments = [str(objects[0])]
    for obj in objects[1:]:
        arguments += ["-object", str(obj)]
    arguments += ["-instr-profile=" + str(profile), *map(str, sources)]
    cov = llvm_tool("llvm-cov")
    report = subprocess.check_output([cov, "report", *arguments], text=True)
    print(report)
    (output / "summary.txt").write_text(report, encoding="utf-8")
    exported = subprocess.check_output([cov, "export", *arguments], text=True)
    (output / "coverage.json").write_text(exported, encoding="utf-8")
    subprocess.run([cov, "show", *arguments, "-format=html", "-output-dir=" + str(output / "html")], check=True)
    files = {Path(entry["filename"]).resolve(): entry["summary"]["lines"]
             for dataset in json.loads(exported)["data"] for entry in dataset["files"]}
    # Explicit inputs prevent new/missing core files from silently disappearing from the denominator.
    assert set(engine.glob("*.cpp")) == set(sources[:5]), "Add every new core source to coverage"
    covered = total = 0
    failed = []
    for source in sources:
        lines = files[source.resolve()]
        covered += lines["covered"]
        total += lines["count"]
        if not lines["count"] or lines["covered"] / lines["count"] <= 0.80:
            failed.append(source.name)
    percentage = covered / total * 100
    print(f"Production line coverage: {percentage:.2f}% ({covered}/{total}); required >80% overall and per file")
    if failed or percentage <= 80:
        sys.exit("Coverage threshold failed: " + ", ".join(failed))


if __name__ == "__main__":
    main()
