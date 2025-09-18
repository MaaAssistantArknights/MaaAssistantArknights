<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet version="2.0"
  xmlns:html="http://www.w3.org/TR/REC-html40"
  xmlns:sitemap="http://www.sitemaps.org/schemas/sitemap/0.9"
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
  <xsl:output method="html" version="1.0" encoding="UTF-8" indent="yes" />
  <xsl:template match="/">
    <html xmlns="http://www.w3.org/1999/xhtml">
      <head>
        <title>XML Sitemap</title>
        <meta http-equiv="Content-Type" content="text/html; charset=utf-8" />
        <meta name="viewport" content="width=device-width, initial-scale=1.0;" />
        <style>
          :root {
            --bg-color: #f8f8f8;
            --bg-color-secondary: #fff;
            --text-color: #2c3e50;
            --border-color: #eaecef;
            --brand-color: #3eaf7c;

            color-scheme: light dark;
          }

          @media (prefers-color-scheme: dark) {
            :root {
              --bg-color: #0d1117;
              --bg-color-secondary: #161b22;
              --text-color: #ccc;
              --border-color: #30363d;
            }
          }

          html,
          body {
            margin: 0;
            padding: 0;
            background: var(--bg-color);
          }

          body {
            min-height: 100vh;
            color: var(--text-color);
            text-align: center;
          }

          #content {
            max-width: 960px;
            margin: 0 auto;
          }

          h1 {
            margin-top: 1rem;
            font-size: 2rem;
          }

          @media (max-width: 419px) {
            h1 {
              font-size: 1.5rem;
            }
          }

          a {
            color: var(--text-color);
            font-weight: 500;
            overflow-wrap: break-word;
          }

          table {
            width: 100%;
            border-radius: 0.5rem;
            border-collapse: collapse;
            text-align: center;
            overflow: hidden;
          }

          @media (max-width: 419px) {
            table {
              border-radius: 0;
            }
          }

          th {
            min-width: 3.5em;
            padding: 0.6em 1em;

            background-color: var(--brand-color);
            color: var(--bg-color);

            font-weight: bold;
            font-size: 1rem;
          }

          @media (max-width: 719px) {
            th {
              font-size: 0.875rem;
            }
          }

          th:first-child {
            text-align: start;
          }

          tr:nth-child(odd) {
            background: var(--bg-color-secondary);
          }

          tr:hover {
            background-color: #e8e8e8;
          }

          @media (prefers-color-scheme: dark) {
            tr:hover {
              background-color: #333;
            }
          }

          td {
            padding: 0.6em 1em;
          }

          @media (max-width: 719px) {
            td {
              font-size: 0.75rem;
            }
          }

          td:first-child {
            text-align: start;
          }

          footer {
            margin-top: 0.75rem;
            padding: 0.25rem;

            color: #888;

            font-size: 0.75rem;
            text-align: center;
          }
        </style>
      </head>
      <body>
        <div id="content">
          <h1>Sitemap</h1>
          <table>
            <thead>
              <tr>
                <th>
                  <xsl:value-of select="concat('URL (', count(sitemap:urlset/sitemap:url), ')')" />
                </th>
                <th>Priority</th>
                <th>Change Frequency</th>
                <th>Last Updated Time</th>
              </tr>
            </thead>
            <tbody>
              <xsl:variable name="lower" select="'abcdefghijklmnopqrstuvwxyz'" />
              <xsl:variable name="upper" select="'ABCDEFGHIJKLMNOPQRSTUVWXYZ'" />
              <xsl:for-each select="sitemap:urlset/sitemap:url">
                <tr>
                  <td>
                    <xsl:variable name="itemURL">
                      <xsl:value-of select="sitemap:loc" />
                    </xsl:variable>
                    <a href="{$itemURL}" target="_blank">
                      <xsl:value-of select="sitemap:loc" />
                    </a>
                  </td>
                  <td>
                    <xsl:choose>
                      <xsl:when test="sitemap:priority">
                        <xsl:value-of select="concat(sitemap:priority*100,'%a')" />
                      </xsl:when>
                      <xsl:otherwise>
                        <xsl:text>0.5</xsl:text>
                      </xsl:otherwise>
                    </xsl:choose>
                  </td>
                  <td>
                    <xsl:choose>
                      <xsl:when test="sitemap:changefreq">
                        <xsl:value-of select="concat(translate(substring(sitemap:changefreq, 1, 1),concat($lower, $upper),concat($upper, $lower)),substring(sitemap:changefreq, 2))" />
                      </xsl:when>
                      <xsl:otherwise>
                        <xsl:text>-</xsl:text>
                      </xsl:otherwise>
                    </xsl:choose>
                  </td>
                  <td>
                    <xsl:value-of select="concat(substring(sitemap:lastmod,0,11),concat(' ', substring(sitemap:lastmod,12,5)))" />
                  </td>
                </tr>
              </xsl:for-each>
            </tbody>
          </table>
        </div>
        <footer>
          Generatd by <a href="https://ecosystem.vuejs.press/plugins/sitemap/">@vuepress/plugin-sitemap</a>
        </footer>
      </body>
    </html>
  </xsl:template>
</xsl:stylesheet>
