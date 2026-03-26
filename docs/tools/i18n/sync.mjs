#!/usr/bin/env node
/**
 * MAA 文档同步脚本
 * 检测 zh-cn 的变更并同步到其他语言
 */

import fs from 'fs/promises';
import path from 'path';
import { fileURLToPath } from 'url';
import { glob } from 'glob';
import chalk from 'chalk';
import { execSync } from 'child_process';
import dotenv from 'dotenv';

const __dirname = path.dirname(fileURLToPath(import.meta.url));
dotenv.config({ path: path.join(__dirname, '../../.env') });

const SOURCE_LANG = process.env.SOURCE_LANG || 'zh-cn';
const TARGET_LANGS = (process.env.TARGET_LANGS || 'en-us,zh-tw,ja-jp,ko-kr').split(',');

const LANG_NAMES = {
  'en-us': 'English',
  'zh-tw': '繁體中文',
  'ja-jp': '日本語',
  'ko-kr': '한국어',
};

// 排除的文件（不需要同步）
const EXCLUDED_FILES = [
  'i18n-guide.md',  // 国际化指南只需简中版本
];

/**
 * 获取文件修改时间
 */
async function getMtime(filePath) {
  try {
    const stat = await fs.stat(filePath);
    return stat.mtime;
  } catch {
    return null;
  }
}

/**
 * 检查文件是否需要同步
 */
async function checkSyncStatus(sourceFile, targetFile) {
  const sourceMtime = await getMtime(sourceFile);
  const targetMtime = await getMtime(targetFile);
  
  if (!sourceMtime) return { needsSync: false, reason: 'Source not found' };
  if (!targetMtime) return { needsSync: true, reason: 'Target missing' };
  
  // 如果源文件比目标文件新，需要同步
  if (sourceMtime > targetMtime) {
    const diffMs = sourceMtime - targetMtime;
    const diffMins = Math.floor(diffMs / 60000);
    return { 
      needsSync: true, 
      reason: `Source is newer by ${diffMins} min`,
      sourceMtime,
      targetMtime,
    };
  }
  
  return { needsSync: false, reason: 'Up to date' };
}

/**
 * 扫描需要同步的文件
 */
async function scanFiles() {
  const sourceDir = path.join(process.cwd(), SOURCE_LANG);
  const allFiles = await glob('**/*.md', { cwd: sourceDir, absolute: true });
  
  const results = [];
  
  for (const sourceFile of allFiles) {
    const relativePath = path.relative(sourceDir, sourceFile);
    const filename = path.basename(relativePath);
    
    // 检查是否在排除列表中
    if (EXCLUDED_FILES.some(excluded => filename === excluded || relativePath.includes(`/${excluded}`))) {
      continue;
    }
    
    const fileResult = {
      file: relativePath,
      source: sourceFile,
      targets: {},
    };
    
    let needsSyncAny = false;
    
    for (const lang of TARGET_LANGS) {
      const targetFile = path.join(process.cwd(), lang, relativePath);
      const status = await checkSyncStatus(sourceFile, targetFile);
      fileResult.targets[lang] = status;
      
      if (status.needsSync) {
        needsSyncAny = true;
      }
    }
    
    if (needsSyncAny) {
      results.push(fileResult);
    }
  }
  
  return results;
}

/**
 * 获取 git 变更的文件
 */
function getGitChangedFiles() {
  try {
    const output = execSync('git diff --name-only HEAD', { 
      cwd: process.cwd(),
      encoding: 'utf-8',
    });
    return output.trim().split('\n').filter(f => f.startsWith(`${SOURCE_LANG}/`));
  } catch {
    return [];
  }
}

/**
 * 显示同步报告
 */
function printReport(files) {
  if (files.length === 0) {
    console.log(chalk.green('✨ All files are up to date!'));
    return;
  }
  
  console.log(chalk.yellow(`\n📋 Found ${files.length} file(s) needing sync:\n`));
  
  for (const item of files) {
    console.log(chalk.cyan(item.file));
    
    for (const [lang, status] of Object.entries(item.targets)) {
      if (status.needsSync) {
        console.log(`  ${chalk.red('•')} ${LANG_NAMES[lang]}: ${status.reason}`);
      } else {
        console.log(`  ${chalk.green('✓')} ${LANG_NAMES[lang]}: ${status.reason}`);
      }
    }
    console.log();
  }
}

/**
 * 生成翻译命令
 */
function printCommands(files) {
  console.log(chalk.blue('🔧 Quick commands to sync:\n'));
  
  for (const item of files) {
    const relativePath = item.file;
    const dir = path.dirname(relativePath);
    const pattern = dir === '.' ? '*.md' : `${dir}/**/*.md`;
    
    for (const lang of TARGET_LANGS) {
      if (item.targets[lang]?.needsSync) {
        console.log(chalk.gray(`# Translate to ${LANG_NAMES[lang]}`));
        console.log(chalk.white(`node tools/i18n/translate.mjs ${lang} "${pattern}"\n`));
      }
    }
  }
}

/**
 * 主函数
 */
async function main() {
  const args = process.argv.slice(2);
  const command = args[0] || 'check';
  
  console.log(chalk.blue('🔍 MAA Docs i18n Sync Tool'));
  console.log(chalk.gray(`Source: ${SOURCE_LANG} | Targets: ${TARGET_LANGS.map(l => LANG_NAMES[l]).join(', ')}\n`));
  
  switch (command) {
    case 'check': {
      const files = await scanFiles();
      printReport(files);
      
      if (files.length > 0) {
        console.log(chalk.gray('─'.repeat(50)));
        printCommands(files);
      }
      break;
    }
    
    case 'git': {
      const changedFiles = getGitChangedFiles();
      if (changedFiles.length === 0) {
        console.log(chalk.green('No git changes detected in source language.'));
        break;
      }
      
      console.log(chalk.yellow(`Git changed files (${changedFiles.length}):`));
      changedFiles.forEach(f => console.log(`  • ${f}`));
      
      // 找出需要同步的目标
      const allFiles = await scanFiles();
      const relevantFiles = allFiles.filter(f => 
        changedFiles.some(cf => cf.includes(f.file))
      );
      
      if (relevantFiles.length > 0) {
        console.log(chalk.yellow(`\nRelated files needing sync:`));
        printReport(relevantFiles);
      }
      break;
    }
    
    case 'init': {
      // 为缺失的文件创建初始版本
      const files = await scanFiles();
      const missingFiles = files.filter(f => 
        Object.entries(f.targets).some(([lang, status]) => 
          status.needsSync && status.reason === 'Target missing'
        )
      );
      
      if (missingFiles.length === 0) {
        console.log(chalk.green('No missing files to initialize.'));
        break;
      }
      
      console.log(chalk.yellow(`Found ${missingFiles.length} missing file(s)\n`));
      
      for (const item of missingFiles) {
        for (const lang of TARGET_LANGS) {
          if (item.targets[lang]?.reason === 'Target missing') {
            const targetPath = path.join(process.cwd(), lang, item.file);
            await fs.mkdir(path.dirname(targetPath), { recursive: true });
            
            // 复制源文件作为初始版本
            const sourceContent = await fs.readFile(item.source, 'utf-8');
            await fs.writeFile(targetPath, sourceContent, 'utf-8');
            
            console.log(chalk.green(`✓ Created: ${lang}/${item.file}`));
          }
        }
      }
      break;
    }
    
    default:
      console.log(chalk.yellow('Usage:'));
      console.log('  node sync.mjs check    # Check sync status (default)');
      console.log('  node sync.mjs git      # Check git changed files');
      console.log('  node sync.mjs init     # Initialize missing files');
  }
}

main().catch(console.error);