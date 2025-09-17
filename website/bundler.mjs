import * as fs from 'fs';

const websiteBasePath = './dist';
const docsBasePath = '../docs/dist';

const maaProjectLocationMapping = [
  {
    from: './apps/web/dist',
    to: `${websiteBasePath}`,
  },
  {
    from: '../docs/.vuepress/dist',
    to: `${docsBasePath}`,
  },
  {
    from: '../docs/staticwebapp.config.json',
    to: `${docsBasePath}/staticwebapp.config.json`,
  },
];

console.log(`[INF] Removing ${websiteBasePath}`);
fs.rmSync(websiteBasePath, { recursive: true, force: true });
console.log(`[INF] Removing ${docsBasePath}`);
fs.rmSync(docsBasePath, { recursive: true, force: true });

maaProjectLocationMapping.forEach(({ from, to }) => {
  if (fs.existsSync(from) === false) {
    console.error(`[ERR] ${from} does not exist`);
    process.exit(-1);
  }
  console.log(`[INF] Copy ${from} to ${to}`);
  fs.cpSync(from, to, { recursive: true });
});
