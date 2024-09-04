import fs from "node:fs";
import path from "node:path";
import { exec as child_process_exec } from "node:child_process";
import { promisify } from "node:util";

const exec = promisify(child_process_exec);
const log = (...args) => console.info(new Date().toLocaleString("zh-Hans-CN"), ...args);

log("Check git status...");
log(`git status:\n${(await exec("git status")).stdout}`);
log("Start to diff the file changes...");
const { stdout } = await exec("git diff --numstat HEAD");
log(`Diff:\n${stdout}`);
let diff = false;
let hasPngDiff = false;
const listPerServer = {};
const mainlandChina = Symbol("mainlandChina");
for (const line of stdout.split(/\r*\n+/)) {
    const [added, deleted, pathname] = line.split(/\s+/);
    const server = pathname.startsWith("resource/global/") ? pathname.split("/")[2] : mainlandChina;
    if (!Reflect.has(listPerServer, server)) {
        listPerServer[server] = [];
    }
    listPerServer[server].push([added, deleted, pathname]);
}
log("Parsed diff:", listPerServer);
for (const [server, list] of Object.entries(listPerServer)) {
    log(`Server: ${server}`);
    let localDiff = false;
    let localHasPngDiff = false;
    for (const [added, deleted, pathname] of list) {
        if (path.posix.extname(pathname) === ".png") {
            localHasPngDiff = true;
            localDiff = true;
            log(`PNG changed: ${pathname}, valid.`);
            continue;
        }
        if (added > 1 || added !== deleted) {
            log(`File changed: ${pathname}, valid.`);
            localDiff = true;
            continue;
        }
        if (path.posix.basename(pathname) === "version.json") {
            const { stdout: versionDiff } = await exec(`git diff --unified=0 HEAD -- ${pathname}`);
            const addedLine = versionDiff.split(/\r*\n+/).filter((line) => line.startsWith("+") && !line.startsWith("+++ "));
            const deletedLine = versionDiff.split(/\r*\n+/).filter((line) => line.startsWith("-") && !line.startsWith("--- "));
            if (
                addedLine.length === 1 && /^\s*"last_updated":/.test(addedLine[0])
                && deletedLine.length === 1 && /^\s*"last_updated":/.test(deletedLine[0])
            ) {
                log(`Version-only changed: ${pathname}, not valid.`);
                continue;
            } else {
                log(`File changed: ${pathname}, valid.`);
                localDiff = true;
                continue;
            }
        }
    }
    if (localHasPngDiff) {
        hasPngDiff = true;
    }
    if (localDiff) {
        diff = true;
    } else {
        log(`No valid changes found in server: ${server}`);
        log("Revert the file changes...");
        for (const [, , pathname] of list) {
            await exec(`git checkout HEAD -- ${pathname}`);
        }
    }
    log("Server check result:", { localHasPngDiff, localDiff });
}
log("Diff check result:", { hasPngDiff, diff });
await fs.promises.appendFile(process.env.GITHUB_OUTPUT, `contains_png=${hasPngDiff}\nchanges=${diff}\n`, { encoding: "utf-8" });
log(`GITHUB_OUTPUT:\n${await fs.promises.readFile(process.env.GITHUB_OUTPUT, { encoding: "utf-8" })}`);
