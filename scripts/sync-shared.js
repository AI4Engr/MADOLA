#!/usr/bin/env node
// sync-shared.js — copy the single-source-of-truth shared/js/ modules into each app's
// tree (web/js/shared/ and app/js/shared/) so they load via plain relative <script>
// paths. This keeps Electron (file://), electron-builder packaging, and the dev servers
// all working without any special /shared/ HTTP route or protocol interception.
//
// Run after editing anything in shared/js/:  node scripts/sync-shared.js
// (Same manual-copy discipline already used for the WASM binaries.)

const fs = require('fs');
const path = require('path');

const repoRoot = path.join(__dirname, '..');
const srcDir = path.join(repoRoot, 'shared', 'js');
const destDirs = [
  path.join(repoRoot, 'web', 'js', 'shared'),
  path.join(repoRoot, 'app', 'js', 'shared'),
];

const AUTO_HEADER =
  '// AUTO-GENERATED COPY — do not edit here. Source of truth: shared/js/' + '\n' +
  '// Regenerate with: node scripts/sync-shared.js' + '\n';

function syncOne(destDir) {
  fs.mkdirSync(destDir, { recursive: true });
  const files = fs.readdirSync(srcDir).filter((f) => f.endsWith('.js'));
  for (const f of files) {
    const body = fs.readFileSync(path.join(srcDir, f), 'utf8');
    fs.writeFileSync(path.join(destDir, f), AUTO_HEADER + body);
    console.log('  ->', path.relative(repoRoot, path.join(destDir, f)));
  }
  return files.length;
}

console.log('Syncing shared/js/ into app trees:');
let total = 0;
for (const d of destDirs) total += syncOne(d);
console.log(`Done: ${total} file(s) copied across ${destDirs.length} target(s).`);
