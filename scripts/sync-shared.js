#!/usr/bin/env node
// sync-shared.js — copy the single-source-of-truth shared/{js,css}/ files into each
// app's tree (web/{js,css}/shared/ and app/{js,css}/shared/) so they load via plain
// relative <script>/<link> paths. This keeps Electron (file://), electron-builder
// packaging, and the dev servers all working without any special /shared/ HTTP route
// or protocol interception.
//
// The generated copies ARE committed (not gitignored): playground.madola.org and
// similar deployments build from a plain git checkout, so anything gitignored here
// 404s in production. .github/workflows/sync-shared.yml keeps them from drifting.
//
// Run after editing anything in shared/js/ or shared/css/:  node scripts/sync-shared.js

const fs = require('fs');
const path = require('path');

const repoRoot = path.join(__dirname, '..');

const KINDS = [
  { name: 'js', ext: '.js', srcDir: path.join(repoRoot, 'shared', 'js'),
    destDirs: [path.join(repoRoot, 'web', 'js', 'shared'), path.join(repoRoot, 'app', 'js', 'shared')] },
  { name: 'css', ext: '.css', srcDir: path.join(repoRoot, 'shared', 'css'),
    destDirs: [path.join(repoRoot, 'web', 'css', 'shared'), path.join(repoRoot, 'app', 'css', 'shared')] },
];

function autoHeader(ext, srcDirRel) {
  const commentOpen = ext === '.css' ? '/*' : '//';
  const commentClose = ext === '.css' ? ' */' : '';
  const line = (text) => `${commentOpen} ${text}${commentClose}`;
  return line(`AUTO-GENERATED COPY — do not edit here. Source of truth: ${srcDirRel}`) + '\n' +
         line('Regenerate with: node scripts/sync-shared.js') + '\n';
}

function syncKind(kind) {
  if (!fs.existsSync(kind.srcDir)) return 0;
  const srcDirRel = path.relative(repoRoot, kind.srcDir).replace(/\\/g, '/') + '/';
  const header = autoHeader(kind.ext, srcDirRel);
  const files = fs.readdirSync(kind.srcDir).filter((f) => f.endsWith(kind.ext));
  let total = 0;
  for (const destDir of kind.destDirs) {
    fs.mkdirSync(destDir, { recursive: true });
    for (const f of files) {
      const body = fs.readFileSync(path.join(kind.srcDir, f), 'utf8');
      fs.writeFileSync(path.join(destDir, f), header + body);
      console.log('  ->', path.relative(repoRoot, path.join(destDir, f)));
      total++;
    }
  }
  return total;
}

console.log('Syncing shared/{js,css}/ into app trees:');
let total = 0;
for (const kind of KINDS) total += syncKind(kind);
console.log(`Done: ${total} file(s) copied.`);
