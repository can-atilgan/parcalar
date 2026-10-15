// Builds the interactive page from page.html + sim.js + the 16-core results tables (data/bench and data/MEBtest).
//   node viz/build.js [OUT_DIR]   (default: viz/dist)
// Writes:
//   parcalar-arama-agaci.html   page body only (for publishing as a Claude artifact)
//   index.html                  standalone page (open directly in a browser)
const fs = require("fs");
const path = require("path");

const here = __dirname;
const root = path.join(here, "..");
const out = process.argv[2] || path.join(here, "dist");

const sim = fs.readFileSync(path.join(here, "sim.js"), "utf8").split("if (typeof module")[0];
const tsv = fs.readFileSync(path.join(root, "results/bench/AMD_Ryzen_7_7800X3D/parcalar/16-cekirdek/ham.tsv"), "utf8");
const results = tsv.trim().split(/\r?\n/).slice(1).map(l => {
  const [f, z, x, st, s, w] = l.split("\t");
  return [f, +z, +x, st, +s, +w];
});

const readTsv = f => fs.existsSync(f) ? fs.readFileSync(f, "utf8").trim().split(/\r?\n/).slice(1).map(l => {
  const [f0, z, x, st, s, w] = l.split("\t");
  return [f0, +z, +x, st, +s, +w];
}) : [];
const meb = readTsv(path.join(root, "results/MEBtest/AMD_Ryzen_7_7800X3D/parcalar/16-cekirdek/ham.tsv"));
const mebIls = readTsv(path.join(root, "results/MEBtest/AMD_Ryzen_7_7800X3D/sezgisel_ILS/tek-cekirdek-10sn/ham.tsv"));
const benchIls = readTsv(path.join(root, "results/bench/AMD_Ryzen_7_7800X3D/sezgisel_ILS/tek-cekirdek-10sn/ham.tsv"));

let page = fs.readFileSync(path.join(here, "page.html"), "utf8");
page = page.replace("// @@SIM@@", () => sim.trim());
page = page.replace("/* @@RESULTS@@ */ []", () => JSON.stringify(results));
page = page.replace("/* @@RESULTS_MEB@@ */ []", () => JSON.stringify(meb));
page = page.replace("/* @@RESULTS_MEB_ILS@@ */ []", () => JSON.stringify(mebIls));
page = page.replace("/* @@RESULTS_BENCH_ILS@@ */ []", () => JSON.stringify(benchIls));

fs.mkdirSync(out, { recursive: true });
fs.writeFileSync(path.join(out, "parcalar-arama-agaci.html"), page);
fs.writeFileSync(path.join(out, "index.html"),
  `<!doctype html>\n<html lang="tr">\n<head>\n<meta charset="utf-8">\n<meta name="viewport" content="width=device-width, initial-scale=1, viewport-fit=cover">\n</head>\n<body style="margin:0">\n${page}\n</body>\n</html>\n`);
console.log(`built ${results.length} bench + ${meb.length} MEBtest + ${mebIls.length} MEBtest ILS + ${benchIls.length} bench ILS result rows -> ${out}`);
