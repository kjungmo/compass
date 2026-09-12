#!/usr/bin/env python3
"""Fast mutation tests for the bounded repository-consistency gate (no builds)."""
import copy
import hashlib
import importlib.util
import json
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest


SPEC = importlib.util.spec_from_file_location("consistency", Path(__file__).with_name("check_repo_consistency.py"))
gate = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(gate)


class ConsistencyTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.groups = gate.raw_groups(gate.ROOT)

    def test_published_numeric_tables(self):
        gate.check_result_tables(gate.ROOT, self.groups)

    def test_variant_identity_is_not_row_order(self):
        table = [["Variant", "Count"], ["-accumulator (argmin)", "72/250"],
                 ["-class correspondence", "71/250"]]
        first = gate.indexed_rows(table, "test", ("accumulator", "correspondence"))
        second = gate.indexed_rows([table[0], *reversed(table[1:])], "test", ("accumulator", "correspondence"))
        self.assertEqual(first, second)
        with self.assertRaisesRegex(gate.CheckError, "duplicate"):
            gate.indexed_rows([table[0], table[1], table[1]], "test", ("accumulator", "correspondence"))

    def test_substring_or_other_metric_cannot_satisfy_cell(self):
        with self.assertRaises(gate.CheckError):
            gate.mean_sd("10.00±0.00", [0.0, 0.0], 2, "zero metric")
        with self.assertRaises(gate.CheckError):
            gate.mean_sd("0.00±0.00 (formerly 10.00)", [0.0, 0.0], 2, "extra text")

    def temporary_tables(self, tmp):
        root = Path(tmp)
        for path in gate.TABLE_SOURCES:
            (root / path).parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(gate.ROOT / path, root / path)
        return root

    def test_every_aggregate_metric_is_checked(self):
        for metric, _ in gate.METRICS:
            with self.subTest(metric=metric):
                groups = copy.deepcopy(self.groups)
                for row in groups["accumulator"]:
                    row[metric] = str(float(row[metric]) + 1)
                with self.assertRaisesRegex(gate.CheckError, metric):
                    gate.check_result_tables(gate.ROOT, groups)

    def test_moving_values_to_another_variant_fails(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = self.temporary_tables(tmp)
            path = root / gate.TABLE_SOURCES[0]
            text = path.read_text(encoding="utf-8").replace("1.33 +/- 1.66", "1.00 +/- 1.20", 1)
            path.write_text(text, encoding="utf-8")
            with self.assertRaisesRegex(gate.CheckError, "full/t_legible_s"):
                gate.check_result_tables(root, self.groups)

    def test_wrong_censor_count_fails(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = self.temporary_tables(tmp)
            path = root / gate.PAPERS[0]
            path.write_text(path.read_text(encoding="utf-8").replace("72/250", "71/250", 1), encoding="utf-8")
            with self.assertRaisesRegex(gate.CheckError, "accumulator/censored"):
                gate.check_result_tables(root, self.groups)

    def test_malformed_or_missing_table_fails(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = self.temporary_tables(tmp)
            path = root / gate.TABLE_SOURCES[0]
            path.write_text(path.read_text(encoding="utf-8").replace("| full (제안) |", "| unknown |", 1), encoding="utf-8")
            with self.assertRaisesRegex(gate.CheckError, "unrecognized variant"):
                gate.check_result_tables(root, self.groups)

    def test_result_summary_maps_counts_to_variants(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            path = root / gate.RESULTS / "README.md"
            path.parent.mkdir(parents=True)
            valid = "| Variant | Censored |\n|---|---|\n| -class correspondence | 71/250 |\n| -accumulator | 72/250 |\n"
            path.write_text(valid, encoding="utf-8")
            gate.check_results_summary(root, self.groups)
            path.write_text(valid.replace("71/250", "72/250"), encoding="utf-8")
            with self.assertRaisesRegex(gate.CheckError, "correspondence"):
                gate.check_results_summary(root, self.groups)

    def role_fixture(self, tmp):
        root = Path(tmp)
        for path in (*gate.TABLE_SOURCES, gate.RESULTS / "README.md"):
            (root / path).parent.mkdir(parents=True, exist_ok=True)
            (root / path).write_text("", encoding="utf-8")
        (root / "paper/arxiv/paper_en.md").write_text(
            "# Canonical English paper\n[Source](main.tex) [PDF](main.pdf)\n", encoding="utf-8")
        (root / gate.PAPERS[1]).write_text(
            "# Korean draft\nNon-canonical. [Source](arxiv/main.tex)\n", encoding="utf-8")
        return root

    def test_retired_censor_count_in_prose_fails(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = self.role_fixture(tmp)
            gate.check_document_roles(root)
            (root / gate.PAPERS[0]).write_text("Both variants have 48/250 censoring.\n", encoding="utf-8")
            with self.assertRaisesRegex(gate.CheckError, "retired 48/250"):
                gate.check_document_roles(root)

    def test_duplicate_english_body_fails(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = self.role_fixture(tmp)
            path = root / "paper/arxiv/paper_en.md"
            path.write_text(path.read_text(encoding="utf-8") + "\n## Abstract\nOld full paper.\n", encoding="utf-8")
            with self.assertRaisesRegex(gate.CheckError, "duplicate manuscript"):
                gate.check_document_roles(root)

    def test_package_versions_must_match_each_other_and_runtime(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            paths = [Path("src") / name / "package.xml" for name in
                     ("compass_core", "compass_nav2", "compass_msgs", "compass_eval")]
            paths.append(Path("sim/package.xml"))
            for path in paths:
                (root / path).parent.mkdir(parents=True)
                (root / path).write_text("<package><version>0.2.0</version></package>\n", encoding="utf-8")
            source = root / "src/compass_core/src/version.cpp"
            source.parent.mkdir()
            source.write_text('const char * version() { return "0.2.0"; }\n', encoding="utf-8")
            smoke = root / "src/compass_core/test/test_smoke.cpp"
            smoke.parent.mkdir(parents=True, exist_ok=True)
            valid_smoke = 'EXPECT_STREQ(compass::version(), "0.2.0");\n'
            smoke.write_text(valid_smoke, encoding="utf-8")
            gate.check_package_versions(root)
            for invalid in (valid_smoke.replace("0.2.0", "0.1.0"), "// missing version assertion\n"):
                smoke.write_text(invalid, encoding="utf-8")
                with self.assertRaisesRegex(gate.CheckError, "test_smoke.cpp"):
                    gate.check_package_versions(root)
            smoke.write_text(valid_smoke, encoding="utf-8")
            source.write_text('const char * version() { return "0.1.0"; }\n', encoding="utf-8")
            with self.assertRaisesRegex(gate.CheckError, "version.cpp"):
                gate.check_package_versions(root)
            (root / paths[0]).write_text("<package><version>0.1.0</version></package>\n", encoding="utf-8")
            with self.assertRaisesRegex(gate.CheckError, "versions disagree"):
                gate.check_package_versions(root)

    def test_relative_links_not_remote_or_headings(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / "exists.md").write_text("# Present\n", encoding="utf-8")
            path = root / "README.md"
            path.write_text("[file](exists.md#unchecked-heading) [head](#x) [remote](https://example.com/missing)\n"
                            "[old](git:deadbeef:old.md)\n```sh\n[example](not-a-link.md)\n```\n"
                            "## Historical git paths\n[old path](removed.md)\n", encoding="utf-8")
            gate.check_local_links(root)
            path.write_text("[broken](missing.md)\n", encoding="utf-8")
            with self.assertRaisesRegex(gate.CheckError, "missing local link"):
                gate.check_local_links(root)

    def artifact_fixture(self, tmp):
        root = Path(tmp)
        (root / "paper/arxiv/figures").mkdir(parents=True)
        (root / "paper/figures").mkdir(parents=True)
        artifacts = {}
        tex = ""
        for i in range(4):
            name = f"figure{i}.pdf"
            tex += rf"\includegraphics{{figures/{name}}}" + "\n"
            for folder in ("paper/figures", "paper/arxiv/figures"):
                (root / folder / name).write_bytes(b"%PDF-figure" + bytes([i]))
            artifacts[f"paper/arxiv/figures/{name}"] = {}
        (root / "paper/arxiv/main.tex").write_text(tex, encoding="utf-8")
        (root / "paper/arxiv/main.pdf").write_bytes(b"%PDF-fixture")
        artifacts.update({"paper/arxiv/main.tex": {}, "paper/arxiv/main.pdf": {}})
        for name in artifacts:
            artifacts[name]["sha256"] = hashlib.sha256((root / name).read_bytes()).hexdigest()
        (root / "paper/arxiv/artifact_manifest.json").write_text(
            json.dumps({"schema_version": 1, "artifacts": artifacts}), encoding="utf-8")
        return root

    def test_manifest_detects_source_pdf_and_figure_mutation(self):
        for path in ("paper/arxiv/main.tex", "paper/arxiv/main.pdf", "paper/arxiv/figures/figure0.pdf"):
            with self.subTest(path=path), tempfile.TemporaryDirectory() as tmp:
                root = self.artifact_fixture(tmp)
                gate.check_artifact_manifest(root)
                file = root / path
                file.write_bytes(file.read_bytes() + b"changed")
                with self.assertRaisesRegex(gate.CheckError, "SHA-256 mismatch"):
                    gate.check_artifact_manifest(root)

    def test_duplicate_figure_bytes_checked_separately(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = self.artifact_fixture(tmp)
            (root / "paper/figures/figure0.pdf").write_bytes(b"different")
            with self.assertRaisesRegex(gate.CheckError, "figure copies differ"):
                gate.check_artifact_manifest(root)

    def test_failure_checks_survive_python_optimization(self):
        command = [sys.executable, "-O", "-c",
                   "import check_repo_consistency as g; g.mean_sd('10.00±0.00', [0, 0], 2, 'intentional failure')"]
        result = subprocess.run(command, cwd=gate.ROOT / "scripts", capture_output=True, text=True, check=False)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("intentional failure", result.stderr)


if __name__ == "__main__":
    unittest.main()
