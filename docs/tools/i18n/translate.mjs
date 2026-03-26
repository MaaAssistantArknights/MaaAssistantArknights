#!/usr/bin/env node
/**
 * MAA 文档 AI 翻译脚本
 * 使用 OpenAI SDK 兼容 API 自动翻译 Markdown 文档
 */

import fs from 'fs/promises';
import path from 'path';
import { fileURLToPath } from 'url';
import OpenAI from 'openai';
import dotenv from 'dotenv';
import chalk from 'chalk';
import { glob } from 'glob';
import matter from 'gray-matter';

// 加载环境变量
const __dirname = path.dirname(fileURLToPath(import.meta.url));
dotenv.config({ path: path.join(__dirname, '../../.env') });

// 语言配置
const LANG_NAMES = {
  'en-us': 'English',
  'zh-tw': '繁體中文',
  'ja-jp': '日本語',
  'ko-kr': '한국어',
};

// 排除的文件（不需要翻译）
const EXCLUDED_FILES = [
  'i18n-guide.md',  // 国际化指南只需简中版本
];

// 验证 OPENAI_API_KEY
if (!process.env.OPENAI_API_KEY) {
  console.error(
    chalk.red(
      '[i18n] Missing OPENAI_API_KEY environment variable. ' +
        'Please set it in your environment or .env file before running docs/tools/i18n/translate.mjs.'
    )
  );
  process.exit(1);
}

// 初始化 OpenAI 客户端
const openai = new OpenAI({
  apiKey: process.env.OPENAI_API_KEY,
  baseURL: process.env.OPENAI_BASE_URL || 'https://api.openai.com/v1',
});

const MODEL = process.env.OPENAI_MODEL || 'gpt-4o-mini';
const TEMPERATURE = parseFloat(process.env.OPENAI_TEMPERATURE || '0.3');
const MAX_TOKENS = parseInt(process.env.OPENAI_MAX_TOKENS || '4096');

/**
 * 翻译文本
 * @param {string} text - 要翻译的文本
 * @param {string} targetLang - 目标语言代码
 * @param {object} glossary - 术语表
 * @returns {Promise<string>}
 */
async function translateText(text, targetLang, glossary = {}) {
  if (!text.trim()) return text;

  const langName = LANG_NAMES[targetLang] || targetLang;
  
  // 构建术语表提示
  let glossaryPrompt = '';
  if (Object.keys(glossary).length > 0) {
    glossaryPrompt = '\n\n术语表（必须按以下对应关系翻译）：\n' + 
      Object.entries(glossary)
        .map(([cn, trans]) => `- ${cn} → ${trans[targetLang] || trans['en-us'] || cn}`)
        .join('\n');
  }

  const systemPrompt = `You are a professional translator translating technical documentation for MAA (MaaAssistantArknights), an Arknights game assistant tool.

Translate the following Markdown content from Chinese to ${langName}.

Rules:
1. Keep all Markdown syntax unchanged (headers, lists, code blocks, links, etc.)
2. Keep frontmatter values in their original language, only translate content
3. Keep code snippets and commands untranslated
4. Keep URLs and file paths unchanged
5. Keep icon names and special tags unchanged
6. Translate naturally and professionally for game/tech context${glossaryPrompt}`;

  try {
    const response = await openai.chat.completions.create({
      model: MODEL,
      messages: [
        { role: 'system', content: systemPrompt },
        { role: 'user', content: text },
      ],
      temperature: TEMPERATURE,
      max_tokens: MAX_TOKENS,
    });

    return response.choices[0].message.content.trim();
  } catch (error) {
    console.error(chalk.red(`Translation failed: ${error.message}`));
    throw error;
  }
}

/**
 * 翻译单个文件
 * @param {string} sourcePath - 源文件路径
 * @param {string} targetLang - 目标语言
 * @param {object} glossary - 术语表
 */
async function translateFile(sourcePath, targetLang, glossary) {
  const content = await fs.readFile(sourcePath, 'utf-8');
  const parsed = matter(content);
  
  // 翻译内容部分（保留 frontmatter）
  const translatedContent = await translateText(parsed.content, targetLang, glossary);
  
  // 构建目标文件路径
  const relativePath = path.relative(
    path.join(process.cwd(), 'zh-cn'),
    sourcePath
  );
  const targetPath = path.join(process.cwd(), targetLang, relativePath);
  
  // 确保目录存在
  await fs.mkdir(path.dirname(targetPath), { recursive: true });
  
  // 写入文件
  const output = matter.stringify(translatedContent, parsed.data);
  await fs.writeFile(targetPath, output, 'utf-8');
  
  console.log(chalk.green(`✓ Translated: ${relativePath} → ${targetLang}`));
  return targetPath;
}

/**
 * 加载术语表
 */
async function loadGlossary() {
  const glossaryPath = path.join(process.cwd(), 'tools', 'i18n', 'glossary.json');
  try {
    const content = await fs.readFile(glossaryPath, 'utf-8');
    return JSON.parse(content);
  } catch {
    return {};
  }
}

/**
 * 主函数
 */
async function main() {
  const args = process.argv.slice(2);
  const targetLang = args[0];
  const filePattern = args[1] || '**/*.md';
  
  if (!targetLang) {
    console.log(chalk.yellow('Usage: node translate.mjs <target-lang> [file-pattern]'));
    console.log(chalk.yellow('Example: node translate.mjs en-us "develop/*.md"'));
    console.log(chalk.yellow('\nSupported languages: ' + Object.keys(LANG_NAMES).join(', ')));
    process.exit(1);
  }
  
  if (!LANG_NAMES[targetLang]) {
    console.error(chalk.red(`Unsupported language: ${targetLang}`));
    console.error(chalk.red('Supported: ' + Object.keys(LANG_NAMES).join(', ')));
    process.exit(1);
  }
  
  console.log(chalk.blue(`🌐 Translating to ${LANG_NAMES[targetLang]}...`));
  console.log(chalk.gray(`Model: ${MODEL}`));
  
  const glossary = await loadGlossary();
  const sourceDir = path.join(process.cwd(), 'zh-cn');
  let files = await glob(filePattern, { cwd: sourceDir, absolute: true });
  
  // 过滤排除的文件
  files = files.filter(file => {
    const filename = path.basename(file);
    const isExcluded = EXCLUDED_FILES.some(excluded => 
      filename === excluded || file.includes(`/${excluded}`)
    );
    if (isExcluded) {
      console.log(chalk.gray(`⊘ Skipped (excluded): ${path.relative(sourceDir, file)}`));
    }
    return !isExcluded;
  });
  
  if (files.length === 0) {
    console.log(chalk.yellow('No files found to translate.'));
    return;
  }
  
  console.log(chalk.blue(`Found ${files.length} file(s) to translate\n`));
  
  // 并发控制翻译
  let CONCURRENCY = parseInt(process.env.CONCURRENCY || '3', 10);
  if (Number.isNaN(CONCURRENCY) || CONCURRENCY <= 0) {
    console.warn(chalk.yellow(`[i18n] Invalid CONCURRENCY value "${process.env.CONCURRENCY}", using default: 3`));
    CONCURRENCY = 3;
  }
  const queue = [...files];
  const results = [];
  
  async function worker() {
    while (queue.length > 0) {
      const file = queue.shift();
      try {
        await translateFile(file, targetLang, glossary);
        results.push({ success: true, file });
      } catch (error) {
        console.error(chalk.red(`✗ Failed to translate ${file}: ${error.message}`));
        results.push({ success: false, file, error });
      }
    }
  }
  
  // 启动多个 worker
  await Promise.all(Array(CONCURRENCY).fill().map(worker));
  
  const successCount = results.filter(r => r.success).length;
  const failCount = results.length - successCount;
  
  console.log(chalk.green(`\n✨ Translation complete! ${successCount} success, ${failCount} failed`));
}

main().catch(console.error);