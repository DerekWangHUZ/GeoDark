#pragma once

#include <string>

namespace geodark {

// 本文件为单 HTML 内嵌 UI（HTML+CSS+JS raw string），经 WebView2 NavigateToString
// 加载。材质 token 与结构规范来源 macUI-SolidGlass 模板本机副本
// （web/src/macos/chrome.css + tokens.css），复制/对齐日期 2026-11-22。
//
// 桥协议不变：JS→C++ postMessage({action})，既有 10 个 action 与字段原样保留；
// C++→JS PostWebMessageAsJson（state/config/theme/toast/switch_view），新增
// {"type":"window","maximized":bool} 只读通知（html.is-maximized 同步归零圆角）。
//
// 结构对齐模板规范（S 清单）：
//   S1 外壳链路 .window→.main→.page→.page-inner→.toolbar（52px leading title1）。
//   S2 同心圆角 32=12+20：窗口 32px（C++ SetWindowRgn 裁角）+ html.is-maximized 归零。
//   S3 悬浮侧栏五段：fixed+12px 外边距+20px 圆角，chrome(40px)→header→搜索钮→
//      sidebar__scroll→footer(border-top)，environment-card 保留在 footer。
//   S4 胶囊导航：999px 胶囊+33px 行高+glass-state 选中（中性，不再业务蓝染色）。
//   S5 红绿灯：几何沿用，补 aria-label。
//   S6 自绘滚动条：.page 原生滚动条隐藏，mac-scrollbar 逻辑内嵌（thumb 5px→7px、
//      避让 44px、rAF+拖拽+轨道点击，移植自 PortPilot mac-scrollbar.js）。
//   S7 HIG 排版：9 档 --font-* token + --track-* tracking；字体栈 Geist 回退系统栈
//      （零构建链，不引字体文件）。
//   S8 页面范式：page-head→toolbar 模式；section-title 17px 分节；表单「标题-描述-
//      控件」三段式（模板 settings 页 Panel/Field 结构）。
//   S10 无障碍：:focus-visible 焦点环 + prefers-reduced-transparency 双降级。
//   S11 命令面板：⌘K/Ctrl+K + 侧栏搜索钮，动作全部复用既有 action（移植自
//      PortPilot command-palette.js），popover 最厚档。
inline std::string get_ui_html() {
    std::string html;
    html.reserve(65536);

    // Part 1: Head、启动脚本与 token（S7 排版 / S2 几何）
    html += R"raw(<!DOCTYPE html>
<html lang="zh-CN">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>GeoDark 设置</title>
  <script>
    // 启动回退（宿主消息到达前的起点，之后一律以 C++ 推送为准）：
    //   1) prefers-color-scheme → html.dark 初始猜测；
    //   2) prefers-reduced-transparency / prefers-reduced-motion → 双降级 class；
    //   3) 上次已知的最大化状态 → html.is-maximized（避免还原瞬间闪现 32px 圆角）。
    (function () {
      var root = document.documentElement;
      var dark = false;
      try { dark = window.matchMedia("(prefers-color-scheme: dark)").matches; } catch (e) { dark = false; }
      if (dark) root.classList.add("dark");
      try {
        if (window.matchMedia("(prefers-reduced-transparency: reduce)").matches) {
          root.classList.add("is-reduced-transparency");
        }
        if (window.matchMedia("(prefers-reduced-motion: reduce)").matches) {
          root.classList.add("is-reduced-motion");
        }
        if (window.sessionStorage.getItem("geodark-maximized") === "1") {
          root.classList.add("is-maximized");
        }
      } catch (e) {}
    })();
  </script>
  <style>
    /* ====================================================================
       Solid Glass —— GeoDark 内嵌 UI 材质 + macOS 结构层
       （材质 token 来源 macUI-SolidGlass 模板本机副本，结构对齐
         web/src/macos/chrome.css + tokens.css，日期 2026-11-22）

       Solid Glass 介于平面设计与液态玻璃之间：每一块面都是一块厚实的烟玻璃，
       透出极少——远看是实心，近看才是玻璃。让它是玻璃的，全在边缘：
       顶上一线光（rim）、四周一根发丝线（hairline）、底下收拢的阴影（shadow）。

       四条材质规则（本文件全部遵循）：
         1. 收光在边缘。     rim + hairline + shadow，面上永无渐变。
         2. 厚重，不空灵。   中性灰；模糊只柔化背后之物，不增其色。
         3. 深度即厚度。     panel（最薄）< card < popover（最厚），
                             越厚越不透明。
         4. 状态唯一。       悬停、选中、按住展开，都穿同一小块透明玻璃
                             （glass-state），无第二种选中样式。

       主题机制：显式 .dark class 优先。C++ 每次状态消息都带
       theme.apps（0=深色 1=浅色），JS 据此切换 html 的 .dark class；
       prefers-color-scheme 仅作宿主消息到达前的初始回退。
       ==================================================================== */
    :root {
      color-scheme: light;
      /* S7 字体栈：Geist 回退系统栈（零构建链，不引字体文件，仅改栈）。 */
      --font-stack-text: "Geist", -apple-system, BlinkMacSystemFont, "SF Pro Text",
        "Segoe UI Variable Text", "Segoe UI", "PingFang SC", "Hiragino Sans GB",
        "Microsoft YaHei", sans-serif;
      --font-stack-display: "Geist", -apple-system, BlinkMacSystemFont, "SF Pro Display",
        "Segoe UI Variable Display", "Segoe UI", "PingFang SC", "Hiragino Sans GB",
        "Microsoft YaHei", sans-serif;
      --font-stack-mono: "Cascadia Mono", Consolas, monospace;

      /* ---- S7 HIG 九档文本样式 + tracking（模板 tokens.css 原值）---- */
      --font-large-title-size: 26px;   --font-large-title-lh: 32px;
      --font-title1-size: 22px;        --font-title1-lh: 26px;
      --font-title2-size: 17px;        --font-title2-lh: 22px;
      --font-title3-size: 15px;        --font-title3-lh: 20px;
      --font-headline-size: 13px;      --font-headline-lh: 16px;
      --font-body-size: 13px;          --font-body-lh: 16px;
      --font-callout-size: 12px;       --font-callout-lh: 15px;
      --font-subheadline-size: 11px;   --font-subheadline-lh: 14px;
      --font-footnote-size: 10px;      --font-footnote-lh: 13px;

      --track-10: 0.12px;
      --track-11: 0.06px;
      --track-12: 0px;
      --track-13: -0.08px;
      --track-15: -0.23px;
      --track-17: -0.43px;
      --track-22: -0.26px;
      --track-26: 0.15px;

      /* ---- S2 同心圆角几何：32 = 12 + 20，圆心统一 (32,32) ---- */
      --window-radius: 32px;
      --sidebar-radius: 20px;
      --radius: 12px;
      --radius-control: 9px;

      /* ---- S1/S3 布局尺寸 ---- */
      --sidebar-w: 220px;
      --sidebar-gap: 12px;
      --content-x: 32px;
      --content-right: 24px;
      --page-max: 1180px;
      --toolbar-h: 52px;
      --chrome-h: 40px;

      /* ---- 动效 ---- */
      --ease-out: cubic-bezier(0.25, 0.1, 0.25, 1);
      --dur-fast: 0.12s;
      --dur: 0.18s;

      /* ---- 色彩（oklch token，映射自模板 :root / .dark）----
         surface  ← card        sidebar ← sidebar
         border   ← border      muted   ← muted-foreground
         blue     ← brand/primary 的功能色替代（GeoDark 业务色）*/
      --background: oklch(0.925 0.007 265);
      --foreground: oklch(0.16 0 0);

      /* 厚度三档：panel < card < popover。 */
      --panel: oklch(0.975 0.004 265 / 44%);
      --card: oklch(0.98 0.004 265 / 60%);
      --popover: oklch(0.985 0.003 265 / 96%);

      --secondary: oklch(0.3 0.02 265 / 9%);
      --muted: oklch(0.47 0 0);
      --border: oklch(0.3 0.02 265 / 13%);
      --input-line: oklch(0.3 0.02 265 / 15%);
      --ring: oklch(0.7 0 0);
      /* 强调色机制已移除（官方定调 primary is the ink）：--blue 改为
         官方中性墨面，主按钮/状态/toast 全部回归单色；变量名保留以最小化 diff。 */
      --blue: oklch(0.2 0 0);
      --blue-dark: oklch(0.32 0 0);

      /* 边缘收光三件套。rim：顶缘一线光；inner：其内一根更弱的第二线；
         edge：四周发丝线；shade：底下阴影；contact/lift：贴地与抬升两层。 */
      --glass-rim: oklch(1 0 0 / 90%);
      --glass-inner: oklch(1 0 0 / 35%);
      --glass-edge: oklch(0.38 0.03 265 / 10%);
      --popover-edge: oklch(0.38 0.03 265 / 22%);
      --popover-under: oklch(0.38 0.03 265 / 16%);
      --glass-shade: oklch(0.3 0.06 265 / 28%);
      --glass-contact: oklch(0.25 0.03 265 / 6%);
      --glass-lift: oklch(0.25 0.03 265 / 14%);

      /* 小件与「唯一状态」用的透明玻璃。 */
      --glass-control: oklch(1 0 0 / 55%);
      --glass-control-rim: oklch(1 0 0 / 80%);
      --glass-state: oklch(0.3 0.02 265 / 9%);
      --glass-state-rim: oklch(1 0 0 / 85%);
      --glass-state-edge: oklch(0.3 0.02 265 / 13%);
      --hover: oklch(0.3 0.02 265 / 10%);
      --hover-faint: oklch(0.3 0.02 265 / 5.5%);

      /* 主按钮：墨色的实心玻璃板。 */
      --primary-solid: 88%;
      --primary-rim: oklch(1 0 0 / 30%);
      --primary-under: oklch(0 0 0 / 45%);
      --primary-inner: oklch(1 0 0 / 7%);
      --primary-edge: oklch(0.2 0 0 / 30%);

      /* 输入域：一块可敲字的透明玻璃。 */
      --field: oklch(1 0 0 / 60%);
      --field-rim: oklch(1 0 0);
      --field-edge: oklch(0.3 0.02 265 / 14%);
      --focus-line: oklch(0.2 0 0 / 50%);

      /* 凹槽（开关槽/未勾选）：光从反方向落。 */
      --well-shade: oklch(0.3 0.06 265 / 14%);
      --well-rim: oklch(1 0 0 / 85%);

      /* 旋钮：最亮的一块板。 */
      --knob: oklch(1 0 0);
      --knob-shade: oklch(0.3 0.06 265 / 30%);

      /* 红绿灯（macOS 窗口控件官方色）。 */
      --tl-close: #ff5f57;
      --tl-min: #febc2e;
      --tl-max: #28c840;
      --tl-close-glyph: rgba(88, 0, 0, 0.66);
      --tl-min-glyph: rgba(93, 54, 0, 0.68);

      --glass-blur: blur(26px) saturate(180%);
      --glass-blur-control: blur(14px);

      /* 玻璃之下的三处环境光（浅色页才有；深色一律透明）。 */
      --light-1: oklch(0.68 0.13 165 / 7%);
      --light-2: oklch(0.75 0.08 240 / 11%);
      --light-3: oklch(0.68 0.13 165 / 4%);

      /* 业务功能色（GeoDark：太阳/月亮/状态）。 */
      --sun: oklch(0.75 0.15 75);
      --moon: oklch(0.58 0.16 285);
      --success: oklch(0.58 0.15 152);
      --warn: oklch(0.66 0.16 58);
      --danger: oklch(0.577 0.245 27.325);
      --log-fg: oklch(0.85 0 0);
      --log-dim: oklch(0.52 0 0);
      --log-bg: oklch(0.18 0.005 265);

      /* 兼容旧变量名（内联 style 引用）：surface 旧名 → card 面。 */
      --surface: var(--card);
      --sidebar: oklch(0.975 0.004 265 / 44%);
      --sidebar-foreground: oklch(0.25 0 0);
    }

    /* ---- 深色：材质本来的样子 ------------------------------------------
       平匀无光的页面，玻璃转为烟色。文字近白。 */
    .dark {
      color-scheme: dark;

      --background: oklch(0.16 0 0);
      --foreground: oklch(0.985 0 0);

      --panel: oklch(1 0 0 / 4.5%);
      --card: oklch(0.31 0 0 / 46%);
      --popover: oklch(0.22 0 0 / 96%);

      --secondary: oklch(1 0 0 / 10%);
      --muted: oklch(0.72 0 0);
      --border: oklch(1 0 0 / 11%);
      --input-line: oklch(1 0 0 / 13%);
      --ring: oklch(0.52 0 0);
      /* 深色档墨面：官方 --primary oklch(0.94 0 0) 的等效 */
      --blue: oklch(0.94 0 0);
      --blue-dark: oklch(0.88 0 0);

      --glass-rim: oklch(1 0 0 / 16%);
      --glass-inner: oklch(1 0 0 / 4%);
      --glass-edge: oklch(1 0 0 / 6%);
      --popover-edge: var(--glass-edge);
      --popover-under: oklch(0 0 0 / 0%);
      --glass-shade: oklch(0 0 0 / 60%);
      --glass-contact: oklch(0 0 0 / 10%);
      --glass-lift: oklch(0 0 0 / 22%);

      --glass-control: oklch(1 0 0 / 7%);
      --glass-control-rim: oklch(1 0 0 / 11%);
      --glass-state: var(--glass-control);
      --glass-state-rim: var(--glass-control-rim);
      --glass-state-edge: var(--glass-edge);
      --hover: oklch(1 0 0 / 8%);
      --hover-faint: oklch(1 0 0 / 4%);

      --primary-solid: 86%;
      --primary-rim: oklch(1 0 0);
      --primary-under: oklch(0 0 0 / 22%);
      --primary-inner: oklch(0 0 0 / 7%);
      --primary-edge: oklch(1 0 0 / 26%);

      --field: oklch(1 0 0 / 6%);
      --field-rim: oklch(1 0 0 / 15%);
      --field-edge: oklch(1 0 0 / 8%);
      --focus-line: oklch(1 0 0 / 40%);

      --well-shade: oklch(0 0 0 / 38%);
      --well-rim: oklch(1 0 0 / 9%);

      --knob: oklch(0.97 0 0);
      --knob-shade: oklch(0 0 0 / 55%);

      --glass-blur: blur(26px);

      --light-1: transparent;
      --light-2: transparent;
      --light-3: transparent;

      --sun: oklch(0.8 0.14 75);
      --moon: oklch(0.72 0.14 285);
      --success: oklch(0.74 0.16 152);
      --warn: oklch(0.77 0.15 62);
      --danger: oklch(0.704 0.191 22.216);
      --log-fg: oklch(0.85 0 0);
      --log-dim: oklch(0.52 0 0);
      --log-bg: oklch(0.14 0.005 265);

      --surface: var(--card);
      --sidebar: oklch(0.11 0 0 / 50%);
      --sidebar-foreground: oklch(0.9 0 0);
    }

    /* ---- S10 双降级：降低透明度（媒体查询自动 + 运行时 class 手动）----
       玻璃整体转实心面板，模糊关闭，保住对比度。 */
    @media (prefers-reduced-transparency: reduce) {
      html {
        --glass-blur: none;
        --glass-blur-control: none;
        --panel: oklch(0.97 0.004 265);
        --card: oklch(0.985 0.003 265);
        --popover: oklch(0.99 0.002 265);
        --sidebar: oklch(0.96 0.004 265);
        --field: oklch(1 0 0 / 88%);
      }
      html.dark {
        --panel: oklch(0.19 0 0);
        --card: oklch(0.24 0 0);
        --popover: oklch(0.22 0 0);
        --sidebar: oklch(0.14 0 0);
        --field: oklch(1 0 0 / 10%);
      }
    }
    html.is-reduced-transparency {
      --glass-blur: none;
      --glass-blur-control: none;
      --panel: oklch(0.97 0.004 265);
      --card: oklch(0.985 0.003 265);
      --popover: oklch(0.99 0.002 265);
      --sidebar: oklch(0.96 0.004 265);
      --field: oklch(1 0 0 / 88%);
    }
    html.dark.is-reduced-transparency {
      --panel: oklch(0.19 0 0);
      --card: oklch(0.24 0 0);
      --popover: oklch(0.22 0 0);
      --sidebar: oklch(0.14 0 0);
      --field: oklch(1 0 0 / 10%);
    }

    /* ---- S10 双降级：减弱动效 ---- */
    html.is-reduced-motion,
    html.is-reduced-motion *,
    html.is-reduced-motion *::before,
    html.is-reduced-motion *::after {
      --dur-fast: 0.01s;
      --dur: 0.01s;
      transition-duration: 0.01ms !important;
      animation-duration: 0.01ms !important;
    }

    /* ---- S7 基础排版 ---- */
    * { box-sizing: border-box; user-select: none; }
    html, body {
      margin: 0; width: 100%; height: 100%; overflow: hidden;
    }
    body {
      font-family: var(--font-stack-text);
      font-size: var(--font-body-size);
      line-height: var(--font-body-lh);
      letter-spacing: var(--track-13);
      color: var(--foreground);
      background: transparent;
      font-synthesis: none;
      text-size-adjust: 100%;
      -webkit-text-size-adjust: 100%;
      -webkit-font-smoothing: antialiased;
      text-rendering: optimizeLegibility;
    }
    /* S6：原生滚动条一律隐藏（.page 由自绘滚动条接管）。 */
    ::-webkit-scrollbar { width: 0; height: 0; }
    * { scrollbar-width: none; }
    button, input, select { font: inherit; letter-spacing: inherit; }
    button {
      color: inherit; background: none; border: 0; padding: 0; cursor: pointer;
    }
    h1, h2, h3, p { margin: 0; }
    svg {
      width: 18px; height: 18px; fill: none; stroke: currentColor;
      stroke-width: 1.8; stroke-linecap: round; stroke-linejoin: round;
      shape-rendering: geometricPrecision;
    }

    /* ---- S10 :focus-visible 焦点环 ---- */
    :focus-visible {
      outline: 2px solid var(--ring);
      outline-offset: 2px;
      border-radius: 4px;
    }
    .light:focus-visible { border-radius: 50%; }
    input:focus-visible, select:focus-visible, .cmdk-input:focus-visible {
      outline: none; /* 输入域用边框线聚焦（--focus-line） */
    }
)raw";

    // Part 2: S1 外壳链路 + S2 同心圆角 + 窗口描边
    html += R"raw(
    /* ================= S1/S2 窗口外壳链路 =================
       .window → .main → .page → .page-inner → .toolbar。
       桌面 32px 圆角由 C++ 侧 SetWindowRgn 按 DPI 裁出；这里同步声明
       border-radius，让网页层与系统裁剪一致，html.is-maximized 时归零。 */
    .window {
      position: fixed; inset: 0; overflow: hidden;
      background-color: var(--background);
      background-image:
        radial-gradient(62rem 42rem at 12% -6rem, var(--light-1), transparent 62%),
        radial-gradient(52rem 38rem at 92% 4rem, var(--light-2), transparent 60%),
        radial-gradient(64rem 46rem at 55% 110%, var(--light-3), transparent 62%);
      background-repeat: no-repeat;
      border-radius: var(--window-radius);
    }
    html.is-maximized .window { border-radius: 0; }

    /* 窗口外缘 1px 描边：border-radius: inherit 保持直线与圆角一致；
       最大化时归零并隐藏（模板 .window::after 同款）。 */
    .window::after {
      content: ""; position: absolute; inset: 0; z-index: 90; pointer-events: none;
      border-radius: inherit;
      border: 1px solid oklch(0.2 0 0 / 15%);
    }
    html.dark .window::after { border-color: oklch(1 0 0 / 15%); }
    html.is-maximized .window::after { border-radius: 0; border-color: transparent; }

    .main {
      position: fixed; inset: 0; display: flex; flex-direction: column;
      overflow: hidden; background: transparent;
    }

    /* S6：页面滚动容器隐藏原生滚动条，避免轨道切入右上/右下 32px 圆角。 */
    .page {
      height: 100%; overflow-y: auto; overflow-x: hidden;
      scrollbar-width: none;
    }
    .page::-webkit-scrollbar { width: 0; height: 0; }

    /* S1 内容让位：上 48 / 下 80 / 左 264（220+12+32）/ 右 24，上限 1180。 */
    .page-inner {
      padding-top: 48px;
      padding-bottom: 80px;
      padding-left: calc(var(--sidebar-w) + var(--sidebar-gap) + var(--content-x));
      padding-right: var(--content-right);
    }
    .page-inner > * { max-width: var(--page-max); }

    /* S9③ 内容区顶部 48px 拖拽带：填满工具栏上方整段让位，好抓不易脱手
       （data-draggable 复用既有 window_drag 动作）。工具栏标题区一并可拖。 */
    .drag-strip {
      position: fixed;
      top: 0;
      left: calc(var(--sidebar-w) + var(--sidebar-gap));
      right: 0;
      height: 48px;
      z-index: 30;
    }

    /* S1 工具栏：52px，leading 标题（title1 22px）+ trailing 动作。 */
    .toolbar {
      display: flex; align-items: center; gap: 12px;
      min-height: var(--toolbar-h); padding: 10px 0 4px;
    }
    .toolbar__heading { display: flex; align-items: center; gap: 10px; flex: 1; min-width: 0; }
    .toolbar__title {
      font-family: var(--font-stack-display);
      font-size: var(--font-title1-size); line-height: var(--font-title1-lh);
      font-weight: 700; letter-spacing: var(--track-22);
      color: var(--foreground); min-width: 0;
      white-space: nowrap; overflow: hidden; text-overflow: ellipsis;
    }
    .toolbar__actions { display: flex; align-items: center; gap: 8px; flex: none; }

    .page-subtitle {
      font-size: var(--font-body-size); line-height: var(--font-body-lh);
      letter-spacing: var(--track-13);
      color: var(--muted); margin: 0 0 20px;
    }

    /* S8 分节标题：title2 17px。 */
    .section-title {
      font-family: var(--font-stack-display);
      font-size: var(--font-title2-size); line-height: var(--font-title2-lh);
      font-weight: 700; letter-spacing: var(--track-17);
      color: var(--foreground); margin: 28px 0 10px;
    }
    .section-desc {
      font-size: var(--font-callout-size); line-height: var(--font-callout-lh);
      letter-spacing: var(--track-12);
      color: var(--muted); margin: -4px 0 14px;
    }

    .hidden { display: none !important; }
)raw";

    // Part 3: S3 悬浮侧栏五段 + S4 胶囊导航 + S5 红绿灯
    html += R"raw(
    /* ================= S3 真悬浮玻璃侧栏（五段） =================
       220px 宽、fixed + 12px 外边距 + 20px 圆角；不再是全高 flex 子元素。
       五段自上而下：chrome(40px 红绿灯行) → header(logo) → 搜索钮 →
       sidebar__scroll → footer(border-top)，environment-card 保留在 footer。 */
    .sidebar {
      position: fixed; top: var(--sidebar-gap); left: var(--sidebar-gap); bottom: var(--sidebar-gap);
      width: var(--sidebar-w); z-index: 20;
      display: flex; flex-direction: column;
      border-radius: var(--sidebar-radius);
      background: var(--sidebar);
      -webkit-backdrop-filter: var(--glass-blur); backdrop-filter: var(--glass-blur);
      box-shadow:
        inset 0 1px 0 var(--glass-rim),
        inset 0 0 0 1px var(--glass-inner),
        0 0 0 1px var(--glass-edge),
        0 1px 2px var(--glass-contact),
        0 12px 32px -14px var(--glass-shade);
      overflow: hidden;
    }

    /* 第一段：chrome 40px 行（红绿灯 + 可拖动）。 */
    .sidebar__chrome {
      display: flex; align-items: center; justify-content: space-between;
      height: var(--chrome-h); flex: 0 0 var(--chrome-h);
      padding: 0 16px; user-select: none;
    }

    /* S5 红绿灯：13px 直径、间距 8px、悬停显字形，几何沿用官方值。 */
    .traffic { display: flex; gap: 8px; align-items: center; }
    .light {
      position: relative; width: 13px; height: 13px; display: grid; place-items: center;
      border-radius: 50%; border: 0.5px solid rgba(0, 0, 0, 0.12); padding: 0; cursor: default;
      transition: filter 0.12s ease, transform 0.1s ease;
    }
    .light.close { background: var(--tl-close); }
    .light.min { background: var(--tl-min); }
    .light.max { background: var(--tl-max); }
    .light::before {
      font: 700 10px/1 "Segoe UI", sans-serif; opacity: 0; transform: translateY(-0.4px); transition: opacity 0.1s ease;
    }
    .light.close::before { content: "×"; color: var(--tl-close-glyph); }
    .light.min::before { content: "−"; color: var(--tl-min-glyph); }
    .light.max::before {
      content: ""; width: 8px; height: 8px;
      background: center/contain no-repeat url("data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 10 10'%3E%3Cpath d='M1.7 4.4V1.7h2.7M4.4 1.7 1.7 4.4M8.3 5.6v2.7H5.6M5.6 8.3l2.7-2.7' fill='none' stroke='%23075b16' stroke-width='1.25' stroke-linecap='round' stroke-linejoin='round'/%3E%3C/svg%3E");
    }
    .traffic:hover .light::before { opacity: 1; }
    .light:hover { filter: brightness(0.96); }
    .light:active { filter: brightness(0.88); transform: scale(0.9); }

    /* 第二段：logo header。 */
    .sidebar__header {
      padding: 2px 16px 10px;
      display: flex; align-items: center; gap: 8px; min-width: 0; cursor: default;
    }
    .sidebar__title {
      font-size: var(--font-title3-size); line-height: var(--font-title3-lh);
      font-weight: 600; letter-spacing: var(--track-15);
      color: var(--foreground); white-space: nowrap; overflow: hidden; text-overflow: ellipsis;
    }

    /* 品牌墨块：主按钮那块实心板，缩到一枚图标的大小。 */
    .title-mark {
      display: flex; align-items: center; justify-content: center; width: 24px; height: 24px; border-radius: 28%;
      background: linear-gradient(135deg, oklch(0.75 0.15 75), oklch(0.55 0.19 257)); color: white;
      box-shadow:
        inset 0 1px 0 var(--primary-rim),
        inset 0 -1px 0 var(--primary-under),
        0 1px 2px var(--glass-contact);
      flex: none;
    }
    .title-mark svg { width: 15px; height: 15px; stroke-width: 2.2; }

    /* 第三段：搜索钮（30px 清玻璃，S11 命令面板入口）。 */
    .sidebar__search-wrap { padding: 0 12px 8px; }
    .sidebar__search-btn {
      width: 100%; height: 30px;
      display: flex; align-items: center; gap: 7px; padding: 0 9px;
      border-radius: 9px;
      background: var(--glass-control); color: var(--muted);
      font-size: var(--font-callout-size); line-height: var(--font-callout-lh);
      box-shadow:
        inset 0 1px 0 var(--glass-control-rim),
        0 0 0 1px var(--glass-edge);
      transition: background var(--dur-fast) var(--ease-out), color var(--dur-fast) var(--ease-out);
    }
    .sidebar__search-btn:hover { background: var(--hover); color: var(--foreground); }
    .sidebar__search-btn svg { width: 14px; height: 14px; flex: none; }
    .search-kbd {
      margin-left: auto; padding: 1px 6px; border-radius: 999px;
      background: var(--glass-control); color: var(--muted);
      font-size: var(--font-footnote-size); line-height: var(--font-footnote-lh);
    }

    /* 第四段：sidebar__scroll（导航区，隐藏原生滚动条）。 */
    .sidebar__scroll {
      flex: 1; overflow-y: auto;
      padding: 6px 10px 14px 12px;
      scrollbar-width: none;
    }
    .sidebar__scroll::-webkit-scrollbar { width: 0; height: 0; }

    /* ---- S4 胶囊导航：999px 胶囊 + 33px 行高 + glass-state 唯一状态 ---- */
    .nav-group { display: flex; flex-direction: column; gap: 4px; margin-bottom: 12px; }
    .nav-group:last-child { margin-bottom: 0; }
    .nav-section-title {
      font-size: var(--font-callout-size); line-height: var(--font-callout-lh);
      font-weight: 600; letter-spacing: var(--track-12);
      color: var(--muted); padding: 10px 10px 6px 11px;
    }
    .nav-link {
      display: flex; align-items: center; gap: 9px; width: 100%;
      min-height: 33px; padding: 0 10px 0 11px;
      border-radius: 999px;
      color: var(--foreground);
      font-size: var(--font-body-size); line-height: var(--font-body-lh); letter-spacing: var(--track-13);
      text-align: left;
      transition: background var(--dur-fast) var(--ease-out), box-shadow var(--dur-fast) var(--ease-out),
        color var(--dur-fast) var(--ease-out);
    }
    .nav-link:hover { background: var(--hover); }
    .nav-link:active { transform: scale(0.99); }
    .nav-link.active {
      background: var(--glass-state);
      box-shadow:
        inset 0 1px 0 var(--glass-state-rim),
        0 0 0 1px var(--glass-state-edge);
      color: var(--foreground); font-weight: 500;
    }
    .nav-icon {
      width: 20px; height: 20px; flex: none;
      display: flex; align-items: center; justify-content: center;
      color: var(--muted);
      transition: color var(--dur-fast) var(--ease-out);
    }
    .nav-icon svg { width: 16px; height: 16px; }
    .nav-link.active .nav-icon { color: var(--foreground); }
    .nav-label { flex: 1; min-width: 0; white-space: nowrap; overflow: hidden; text-overflow: ellipsis; }

    /* 第五段：footer（border-top），environment-card 与后台状态 pill 立在这里。 */
    .sidebar__footer {
      padding: 8px 10px 10px 12px;
      border-top: 1px solid var(--border);
    }

    /* 环境 pill：一块透明玻璃小件。 */
    .env-pill {
      width: 100%; min-width: 0; display: flex; align-items: center; gap: 7px; padding: 5px 8px; border-radius: 7px;
      background: var(--glass-control); color: var(--muted); font-size: var(--font-subheadline-size); line-height: var(--font-subheadline-lh); font-weight: 600;
      box-shadow:
        inset 0 1px 0 var(--glass-control-rim),
        inset 0 0 0 1px var(--glass-edge);
    }
    .env-pill span { min-width: 0; overflow: hidden; text-overflow: ellipsis; white-space: nowrap; }
    .env-pill i {
      width: 7px; height: 7px; border-radius: 50%; background: var(--danger);
      box-shadow: 0 0 0 3px color-mix(in oklch, var(--danger) 15%, transparent); flex: 0 0 auto;
    }
    .env-pill.ok i { background: var(--success); box-shadow: 0 0 0 3px color-mix(in oklch, var(--success) 15%, transparent); }

    /* 环境卡片：立在同一块侧栏上的薄板（panel 档，共享其模糊）。 */
    .environment-card {
      margin-top: 8px; padding: 12px; border: 0; border-radius: 10px;
      background: var(--panel); font-size: var(--font-subheadline-size); line-height: var(--font-subheadline-lh);
      box-shadow:
        inset 0 1px 0 var(--glass-inner),
        inset 0 0 0 1px var(--glass-edge);
    }
    .environment-card > div:not(.card-actions) {
      display: flex; align-items: center; gap: 7px; margin-top: 8px; color: var(--muted);
    }
    .environment-card .env-title {
      margin: 0 0 4px !important; color: var(--foreground) !important; font-weight: 650; display: flex; align-items: center;
    }
    .env-title span { flex: 1; }
    .env-title button {
      border: 0; background: transparent; cursor: pointer; color: var(--muted); padding: 2px; border-radius: 4px; display: flex; align-items: center;
      transition: background-color 0.15s, color 0.15s;
    }
    .env-title button:hover { background: var(--hover); color: var(--blue); }
    .environment-card i { width: 7px; height: 7px; border-radius: 50%; background: var(--danger); flex: 0 0 auto; }
    .environment-card i.ok { background: var(--success); }
    .environment-card i.warn { background: var(--warn); }
    .environment-card b { margin-left: auto; font-size: var(--font-footnote-size); font-weight: 550; color: var(--foreground); }

    .card-actions { display: grid; grid-template-columns: 1fr 1fr; gap: 6px; margin-top: 10px; }

    /* 卡片小按钮：透明玻璃（outline 按钮的处理）。 */
    .card-action-btn {
      width: 100%; border: 1px solid var(--border); border-radius: 6px; padding: 6px; color: var(--foreground);
      background: var(--glass-control); font-size: var(--font-footnote-size); font-weight: 600; cursor: pointer;
      display: inline-flex; align-items: center; justify-content: center; gap: 4px;
      transition: background-color 0.15s;
    }
    .card-action-btn:hover:not(:disabled) { background: var(--hover); }
    .card-action-btn:disabled { opacity: 0.45; cursor: default; }
)raw";

    // Part 4: S8 页面组件（按钮/卡片/表单三段式/开关/状态卡/日志）
    html += R"raw(
    /* ================= S8 页面组件 ================= */

    /* 普通按钮：透明玻璃，发丝线是它自己的边框。 */
    .button {
      height: 32px; padding: 0 14px; border-radius: var(--radius-control); border: 1px solid var(--border);
      background: var(--glass-control); color: var(--foreground);
      display: inline-flex; align-items: center; justify-content: center; gap: 6px;
      font-size: var(--font-callout-size); line-height: var(--font-callout-lh); font-weight: 620; cursor: pointer;
      box-shadow:
        inset 0 1px 0 var(--glass-control-rim),
        0 1px 2px var(--glass-contact);
      transition: transform 0.12s ease, box-shadow 0.12s ease, background-color 0.15s ease;
    }
    .button:hover:not(:disabled) {
      transform: translateY(-1px);
      background: var(--hover);
      box-shadow:
        inset 0 1px 0 var(--glass-control-rim),
        0 3px 8px -2px var(--glass-shade);
    }
    .button:active:not(:disabled) {
      transform: scale(0.98);
      box-shadow: inset 0 1px 0 var(--glass-control-rim);
    }
    .button:disabled { opacity: 0.48; cursor: default; }
    .button svg { width: 14px; height: 14px; }

    /* 主按钮：墨色的实心玻璃板——最实的一块。 */
    .button.primary {
      color: white; border-color: color-mix(in oklab, var(--blue) 55%, var(--primary-edge));
      background-color: color-mix(in oklch, var(--blue) var(--primary-solid), transparent);
      box-shadow:
        inset 0 1px 0 var(--primary-rim),
        inset 0 -1px 0 var(--primary-under),
        inset 0 0 0 1px var(--primary-inner),
        0 1px 2px var(--glass-contact),
        0 8px 18px -9px var(--glass-shade);
    }
    .button.primary:hover:not(:disabled) {
      background-color: color-mix(in oklch, var(--blue) calc(var(--primary-solid) - 10%), transparent);
    }

    /* 设置分节 / 状态卡 / 日志卡：card 档（中间厚度）。边框让位，收光三件套接管。 */
    .setting-section {
      padding: 18px 20px; margin-bottom: 14px; background: var(--card); border: 0; border-radius: var(--radius);
      -webkit-backdrop-filter: var(--glass-blur); backdrop-filter: var(--glass-blur);
      box-shadow:
        inset 0 1px 0 var(--glass-rim),
        inset 0 0 0 1px var(--glass-inner),
        0 0 0 1px var(--glass-edge),
        0 1px 2px var(--glass-contact),
        0 12px 32px -14px var(--glass-shade);
    }
    .page-inner .setting-section:last-child { margin-bottom: 0; }

    /* 卡片内标题-描述头（模板 Panel 头的三段式第一段）。 */
    .section-head { margin-bottom: 14px; }
    .section-head__title {
      font-size: var(--font-headline-size); line-height: var(--font-headline-lh);
      font-weight: 650; letter-spacing: var(--track-13); color: var(--foreground);
    }
    .section-head__desc {
      margin-top: 2px;
      font-size: var(--font-subheadline-size); line-height: var(--font-subheadline-lh);
      color: var(--muted);
    }

    /* S8 表单行：「标题-描述-控件」三段式（模板 Field 结构）。 */
    .form-rows > * + * { border-top: 1px solid var(--border); margin-top: 4px; }
    .form-field { display: flex; align-items: center; justify-content: space-between; gap: 16px; padding: 12px 0; }
    .form-field__text { min-width: 0; }
    .form-field__title {
      font-size: var(--font-headline-size); line-height: var(--font-headline-lh);
      font-weight: 600; letter-spacing: var(--track-13); color: var(--foreground);
    }
    .form-field__desc {
      margin-top: 2px;
      font-size: var(--font-subheadline-size); line-height: var(--font-subheadline-lh);
      color: var(--muted);
    }
    .form-field__control { flex: none; width: 240px; }
    .form-field--stack { display: block; padding: 12px 0; }
    .form-field--stack .form-field__control { width: 100%; margin-top: 8px; }
    .two-col { display: grid; grid-template-columns: 1fr 1fr; gap: 14px; }

    /* 输入域：一块可敲字的透明玻璃——聚焦时边框变成更亮的一根线。 */
    input, select {
      width: 100%; height: 30px; padding: 0 10px; outline: 0;
      border: 1px solid var(--field-edge); border-radius: var(--radius-control); background: var(--field);
      color: inherit; font-size: var(--font-body-size); line-height: var(--font-body-lh);
      transition: border-color 0.15s, background-color 0.15s; user-select: text;
    }
    input:focus, select:focus {
      border-color: var(--focus-line); background: var(--field);
    }
    input:disabled, select:disabled { opacity: 0.45; background: var(--secondary); cursor: not-allowed; }
    input[type="number"] { padding-right: 6px; }

    /* 开关行：立着的最薄板（panel 档）；选中行穿唯一状态的玻璃。
       行内自成「标题-描述-开关」三段式。 */
    .check-row {
      display: flex; align-items: center; gap: 12px; padding: 12px; border: 0; cursor: pointer;
      border-radius: 10px; background: var(--panel);
      box-shadow:
        inset 0 1px 0 var(--glass-inner),
        inset 0 0 0 1px var(--glass-edge);
    }
    .check-row + .check-row { margin-top: 10px; }
    .check-row:has(input:checked) {
      background: var(--glass-state);
      box-shadow:
        inset 0 1px 0 var(--glass-state-rim),
        inset 0 0 0 1px var(--glass-state-edge);
    }

    /* 开关：槽是凹进去的（光反向落）；旋钮是最亮的一块板。 */
    .check-row input {
      appearance: none; -webkit-appearance: none; flex: 0 0 auto; width: 36px; height: 20px;
      margin: 0; padding: 0; border: 0; border-radius: 10px; background: var(--secondary); position: relative; cursor: pointer;
      box-shadow:
        inset 0 1px 1.5px var(--well-shade),
        inset 0 -1px 0 var(--well-rim);
      transition: background-color 0.2s;
    }
    /* 勾选后：槽变成墨色的实心小板（半厚度的那一档）。 */
    .check-row input:checked {
      background-color: color-mix(in oklch, var(--success) var(--primary-solid), transparent);
      box-shadow:
        inset 0 1px 0 color-mix(in oklch, oklch(1 0 0) 55%, transparent),
        inset 0 -1px 0 color-mix(in oklch, oklch(0 0 0) 22%, transparent);
    }
    .check-row input::after {
      content: ""; position: absolute; top: 2px; left: 2px; width: 16px; height: 16px;
      border-radius: 50%; background: var(--knob);
      box-shadow:
        inset 0 1px 0 oklch(1 0 0 / 90%),
        inset 0 -1px 0 oklch(0 0 0 / 14%),
        0 1px 2px var(--knob-shade);
      transition: transform 0.2s cubic-bezier(0.2, 0.8, 0.2, 1);
    }
    .check-row input:checked::after { transform: translateX(16px); }
    .check-row .check-text { flex: 1; min-width: 0; }
    .check-row strong { display: block; font-size: var(--font-headline-size); line-height: var(--font-headline-lh); font-weight: 600; }
    .check-row small { display: block; margin-top: 2px; color: var(--muted); font-size: var(--font-subheadline-size); line-height: var(--font-subheadline-lh); font-weight: 400; }
    .form-hint {
      margin-top: 12px; color: var(--muted);
      font-size: var(--font-callout-size); line-height: var(--font-callout-lh);
    }

    .status-grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(240px, 1fr)); gap: 14px; }

    /* 状态卡：card 档，同 setting-section 一块材质。 */
    .status-card {
      background: var(--card); border: 0; border-radius: var(--radius); padding: 16px 18px;
      -webkit-backdrop-filter: var(--glass-blur); backdrop-filter: var(--glass-blur);
      box-shadow:
        inset 0 1px 0 var(--glass-rim),
        inset 0 0 0 1px var(--glass-inner),
        0 0 0 1px var(--glass-edge),
        0 1px 2px var(--glass-contact),
        0 12px 32px -14px var(--glass-shade);
    }
    .status-card-top { display: flex; align-items: center; gap: 10px; margin-bottom: 12px; }
    .status-card-icon {
      width: 32px; height: 32px; border-radius: 8px; display: grid; place-items: center;
      background: var(--glass-state); color: var(--blue);
      box-shadow:
        inset 0 1px 0 var(--glass-state-rim),
        inset 0 0 0 1px var(--glass-state-edge);
    }
    .status-card-icon.sun { color: var(--sun); }
    .status-card-icon.moon { color: var(--moon); }
    .status-card-title { flex: 1; font-size: var(--font-headline-size); line-height: var(--font-headline-lh); font-weight: 650; }
    .status-tag {
      padding: 3px 8px; border-radius: 99px; font-size: var(--font-footnote-size); line-height: var(--font-footnote-lh); font-weight: 650;
      background: color-mix(in oklch, var(--success) 12%, transparent); color: var(--success);
      display: inline-flex; align-items: center; gap: 4px;
    }
    .status-tag i { width: 6px; height: 6px; border-radius: 50%; background: currentColor; }
    .status-tag.warn { background: color-mix(in oklch, var(--warn) 14%, transparent); color: var(--warn); }
    .status-tag.bad { background: color-mix(in oklch, var(--danger) 14%, transparent); color: var(--danger); }
    .status-meta { display: grid; gap: 6px; font-size: var(--font-subheadline-size); line-height: var(--font-subheadline-lh); }
    .status-meta-row { display: flex; justify-content: space-between; color: var(--muted); }
    .status-meta-row b { color: var(--foreground); font-weight: 550; }

    /* 日志卡：厚板（popover 档）上挖出的墨色凹井——凹槽规则，
       光从反向落；文字颜色不变。 */
    .log-toolbar { display: flex; justify-content: flex-end; margin-bottom: 10px; }
    .log-meta {
      font-size: var(--font-footnote-size); line-height: var(--font-footnote-lh); color: var(--muted);
    }
    .log-panel {
      background: var(--log-bg); border-radius: 10px; padding: 14px 0; overflow-y: auto; max-height: 320px; min-height: 200px;
      color: var(--log-fg);
      scrollbar-width: none;
      box-shadow:
        inset 0 1px 1.5px var(--well-shade),
        inset 0 -1px 0 var(--well-rim),
        inset 0 0 0 1px var(--glass-edge);
    }
    .log-panel::-webkit-scrollbar { width: 0; height: 0; }
    .log-line {
      display: flex; gap: 12px; padding: 4px 16px; font-family: var(--font-stack-mono);
      font-size: var(--font-subheadline-size); line-height: 1.5; user-select: text;
    }
    .log-line time { flex: 0 0 65px; color: var(--log-dim); }
    .log-line span { white-space: pre-wrap; overflow-wrap: anywhere; }
    .log-line.cmd span { color: oklch(0.72 0.12 250); }
    .log-line.ok span { color: var(--success); }
    .log-line.warn span { color: var(--warn); }
    .log-line.err span { color: var(--danger); }
)raw";

    // Part 5: Toast + S11 命令面板（popover 最厚档）+ S6 自绘滚动条样式
    html += R"raw(
    /* Toast：浮起的东西是厚玻璃（popover 档），按类型上一道左侧色条。 */
    .toast-stack { position: fixed; z-index: 200; top: 18px; right: 18px; display: flex; flex-direction: column; gap: 8px; pointer-events: none; }
    .toast {
      pointer-events: auto; min-width: 260px; max-width: 380px; padding: 11px 14px;
      border: 0; border-left: 3px solid var(--blue); border-radius: 9px;
      background: var(--popover);
      -webkit-backdrop-filter: var(--glass-blur); backdrop-filter: var(--glass-blur);
      box-shadow:
        inset 0 1px 0 var(--glass-rim),
        inset 0 -1px 0 var(--popover-under),
        0 0 0 1px var(--popover-edge),
        0 24px 60px -18px var(--glass-shade),
        0 6px 16px -6px var(--glass-lift);
      color: var(--foreground); font-size: var(--font-callout-size); line-height: var(--font-callout-lh); font-weight: 500;
      animation: toastIn 0.2s ease-out; transition: opacity 0.2s, transform 0.2s;
    }
    .toast.success { border-left-color: var(--success); }
    .toast.error { border-left-color: var(--danger); }
    .toast.warning { border-left-color: var(--warn); }
    .toast.leaving { opacity: 0; transform: translateX(12px); }
    @keyframes toastIn { from { opacity: 0; transform: translateX(12px); } to { opacity: 1; transform: translateX(0); } }

    /* ================= S11 命令面板（模板 cmdk 的 vanilla 内嵌实现） =================
       弹层用最厚档玻璃（popover）；input 过滤 + 分组列表 + 键盘导航。 */
    .cmdk-mask {
      position: fixed; inset: 0; z-index: 300; display: none;
      align-items: flex-start; justify-content: center; padding-top: 96px;
      background: oklch(0 0 0 / 22%);
    }
    .cmdk-mask.open { display: flex; }
    .cmdk {
      width: min(560px, calc(100% - 48px)); border-radius: 14px; overflow: hidden;
      background: var(--popover);
      -webkit-backdrop-filter: var(--glass-blur); backdrop-filter: var(--glass-blur);
      box-shadow:
        inset 0 1px 0 var(--glass-rim),
        inset 0 -1px 0 var(--popover-under),
        0 0 0 1px var(--popover-edge),
        0 32px 80px -20px var(--glass-shade),
        0 10px 24px -8px var(--glass-lift);
      animation: cmdkIn 0.16s ease-out;
    }
    @keyframes cmdkIn { from { opacity: 0; transform: translateY(-8px) scale(0.98); } to { opacity: 1; transform: translateY(0) scale(1); } }
    .cmdk-input-wrap {
      display: flex; align-items: center; gap: 8px; padding: 12px 14px;
      border-bottom: 1px solid var(--input-line); color: var(--muted);
    }
    .cmdk-input-wrap svg { width: 15px; height: 15px; flex: none; }
    .cmdk-input {
      flex: 1; height: 24px; border: 0; outline: 0; padding: 0; background: transparent;
      color: var(--foreground); font-size: var(--font-body-size); line-height: var(--font-body-lh);
      user-select: text;
    }
    .cmdk-input::placeholder { color: var(--muted); }
    .cmdk-esc {
      padding: 1px 6px; border-radius: 5px; background: var(--glass-control); color: var(--muted);
      font-size: var(--font-footnote-size); line-height: var(--font-footnote-lh);
      box-shadow: 0 0 0 1px var(--glass-edge);
    }
    .cmdk-list { max-height: 320px; overflow-y: auto; padding: 6px; scrollbar-width: none; }
    .cmdk-list::-webkit-scrollbar { width: 0; height: 0; }
    .cmdk-group-heading {
      padding: 8px 10px 4px;
      font-size: var(--font-footnote-size); line-height: var(--font-footnote-lh);
      font-weight: 600; letter-spacing: var(--track-12); color: var(--muted);
    }
    .cmdk-item {
      display: flex; align-items: center; gap: 10px; min-height: 34px; padding: 0 10px;
      border-radius: 8px; cursor: pointer;
      font-size: var(--font-body-size); line-height: var(--font-body-lh);
    }
    .cmdk-item svg { width: 15px; height: 15px; color: var(--muted); flex: none; }
    .cmdk-item.active {
      background: var(--glass-state);
      box-shadow:
        inset 0 1px 0 var(--glass-state-rim),
        0 0 0 1px var(--glass-state-edge);
    }
    .cmdk-item-label { flex: 1; min-width: 0; white-space: nowrap; overflow: hidden; text-overflow: ellipsis; }
    .cmdk-item-hint { font-size: var(--font-footnote-size); line-height: var(--font-footnote-lh); color: var(--muted); flex: none; }
    .cmdk-empty { padding: 24px; text-align: center; color: var(--muted); font-size: var(--font-callout-size); line-height: var(--font-callout-lh); }

    /* ================= S6 自绘悬浮滚动条 =================
       透明轨道 + 胶囊滑块，上下各避让 44px 圆角区域；
       悬停/拖拽时滑块 5px → 7px（模板 .mac-scrollbar 同款）。 */
    .mac-scrollbar {
      position: fixed; top: 44px; bottom: 44px; right: 4px; width: 10px;
      z-index: 45; display: none; pointer-events: none;
    }
    .mac-scrollbar.is-on { display: block; }
    .mac-scrollbar__thumb {
      width: 5px; margin: 0 auto; border-radius: 999px;
      background: oklch(0.35 0 0 / 28%);
      pointer-events: auto; will-change: transform;
      transition: width var(--dur-fast) var(--ease-out), background var(--dur-fast) var(--ease-out);
    }
    html.dark .mac-scrollbar__thumb { background: oklch(0.85 0 0 / 24%); }
    .mac-scrollbar:hover .mac-scrollbar__thumb,
    .mac-scrollbar.is-dragging .mac-scrollbar__thumb {
      width: 7px; background: oklch(0.3 0 0 / 50%);
    }
    html.dark .mac-scrollbar:hover .mac-scrollbar__thumb,
    html.dark .mac-scrollbar.is-dragging .mac-scrollbar__thumb {
      background: oklch(0.92 0 0 / 46%);
    }

    /* 少动效（模板 Motion 节的精神：动只说明有事发生）。 */
    @media (prefers-reduced-motion: reduce) {
      .toast { animation: none; }
      .cmdk { animation: none; }
      .light, .nav-link, .button, .check-row input::after { transition: none; }
    }
  </style>
</head>)raw";

    // Part 6: Body 结构（S1 链路 + S3 五段侧栏 + S8 页面范式 + S5 aria）
    html += R"raw(
<body>
  <div class="window">
    <div class="drag-strip" data-draggable aria-hidden="true"></div>
    <aside class="sidebar">
      <div class="sidebar__chrome" data-draggable>
        <div class="traffic">
          <button class="light close" id="btnWinClose" title="关闭" aria-label="关闭窗口"></button>
          <button class="light min" id="btnWinMin" title="最小化" aria-label="最小化窗口"></button>
          <button class="light max" id="btnWinMax" title="最大化" aria-label="最大化或还原窗口"></button>
        </div>
      </div>
      <div class="sidebar__header" data-draggable>
        <span class="title-mark">
          <svg viewBox="0 0 24 24"><path d="M12 3a9 9 0 1 0 9 9c0-.46-.04-.92-.1-1.36a5.389 5.389 0 0 1-4.4 2.26 5.403 5.403 0 0 1-3.14-9.8c-.44-.06-.9-.1-1.36-.1z"/></svg>
        </span>
        <span class="sidebar__title">GeoDark</span>
      </div>
      <div class="sidebar__search-wrap">
        <button class="sidebar__search-btn" id="btnOpenPalette" type="button" title="命令面板 (Ctrl+K)" aria-label="打开命令面板">
          <svg viewBox="0 0 24 24"><circle cx="11" cy="11" r="7"/><path d="M20 20l-3.5-3.5"/></svg>
          <span>搜索</span>
          <span class="search-kbd">Ctrl K</span>
        </button>
      </div>
      <div class="sidebar__scroll" id="sidebarScroll">
        <div class="nav-group">
          <div class="nav-section-title">工作台</div>
          <button class="nav-link active" id="navStatus" data-view="status" type="button">
            <span class="nav-icon"><svg viewBox="0 0 24 24"><circle cx="12" cy="12" r="4"/><path d="M12 2v2M12 20v2M4.93 4.93l1.41 1.41M17.66 17.66l1.41 1.41M2 12h2M20 12h2M6.34 17.66l-1.41 1.41M19.07 4.93l-1.41 1.41"/></svg></span>
            <span class="nav-label">运行状态</span>
          </button>
        </div>
        <div class="nav-group">
          <div class="nav-section-title">偏好设置</div>
          <button class="nav-link" id="navSettings" data-view="settings" type="button">
            <span class="nav-icon"><svg viewBox="0 0 24 24"><circle cx="12" cy="12" r="3"/><path d="M19.4 15a1.65 1.65 0 0 0 .33 1.82l.06.06a2 2 0 0 1 0 2.83 2 2 0 0 1-2.83 0l-.06-.06a1.65 1.65 0 0 0-1.82-.33 1.65 1.65 0 0 0-1 1.51V21a2 2 0 0 1-2 2 2 2 0 0 1-2-2v-.09A1.65 1.65 0 0 0 9 19.4a1.65 1.65 0 0 0-1.82.33l-.06.06a2 2 0 0 1-2.83 0 2 2 0 0 1 0-2.83l.06-.06a1.65 1.65 0 0 0 .33-1.82 1.65 1.65 0 0 0-1.51-1H3a2 2 0 0 1-2-2 2 2 0 0 1 2-2h.09A1.65 1.65 0 0 0 4.6 9a1.65 1.65 0 0 0-.33-1.82l-.06-.06a2 2 0 0 1 0-2.83 2 2 0 0 1 2.83 0l.06.06a1.65 1.65 0 0 0 1.82.33H9a1.65 1.65 0 0 0 1-1.51V3a2 2 0 0 1 2-2 2 2 0 0 1 2 2v.09a1.65 1.65 0 0 0 1 1.51 1.65 1.65 0 0 0 1.82-.33l.06-.06a2 2 0 0 1 2.83 0 2 2 0 0 1 0 2.83l-.06.06a1.65 1.65 0 0 0-.33 1.82V9a1.65 1.65 0 0 0 1.51 1H21a2 2 0 0 1 2 2 2 2 0 0 1-2 2h-.09a1.65 1.65 0 0 0-1.51 1z"/></svg></span>
            <span class="nav-label">参数设置</span>
          </button>
        </div>
      </div>
      <div class="sidebar__footer">
        <div class="env-pill" id="envPill">
          <i></i>
          <span id="envPillText">后台：检测中</span>
        </div>
        <div class="environment-card">
          <div class="env-title">
            <span>环境与服务</span>
            <button id="btnRefreshEnv" title="刷新状态" aria-label="刷新状态">
              <svg viewBox="0 0 24 24" style="width:13px;height:13px;"><path d="M23 4v6h-6M1 20v-6h6"/><path d="M3.51 9a9 9 0 0 1 14.85-3.36L23 10M1 14l4.64 4.36A9 9 0 0 0 20.49 15"/></svg>
            </button>
          </div>
          <div>
            <i id="envDotLocation" class="ok"></i>
            <span>Windows 定位</span>
            <b id="envValLocation">检测中</b>
          </div>
          <div>
            <i id="envDotSolar" class="ok"></i>
            <span>太阳周期</span>
            <b id="envValSolar">计算中</b>
          </div>
          <div>
            <i id="envDotTheme" class="ok"></i>
            <span>当前主题</span>
            <b id="envValTheme">浅色 / 浅色</b>
          </div>
          <div class="card-actions">
            <button class="card-action-btn" id="btnQuickRefreshLoc">
              <svg viewBox="0 0 24 24" style="width:12px;height:12px;"><path d="M21.5 2v6h-6M21.34 15.57a10 10 0 1 1-.57-8.38l5.67-5.67"/></svg>
              刷新位置
            </button>
            <button class="card-action-btn" id="btnQuickAuth">
              <svg viewBox="0 0 24 24" style="width:12px;height:12px;"><path d="M12 22s8-4 8-10V5l-8-3-8 3v7c0 6 8 10 8 10z"/></svg>
              授权定位
            </button>
          </div>
        </div>
      </div>
    </aside>

    <main class="main">
      <div class="page" id="pageScroller">
        <div class="page-inner">
          <!-- S1 工具栏：52px leading 标题 + trailing 动作（原 page-head 改造）。 -->
          <header class="toolbar" data-draggable>
            <div class="toolbar__heading">
              <h1 class="toolbar__title" id="pageTitle">运行状态</h1>
            </div>
            <div class="toolbar__actions" id="toolbarActionsStatus">
              <button class="button" id="btnManualLight">
                <svg viewBox="0 0 24 24"><circle cx="12" cy="12" r="5"/><path d="M12 1v2M12 21v2M4.22 4.22l1.42 1.42M18.36 18.36l1.42 1.42M1 12h2M21 12h2M4.22 19.78l1.42-1.42M18.36 5.64l1.42-1.42"/></svg>
                浅色
              </button>
              <button class="button" id="btnManualDark">
                <svg viewBox="0 0 24 24"><path d="M21 12.79A9 9 0 1 1 11.21 3 7 7 0 0 0 21 12.79z"/></svg>
                深色
              </button>
              <button class="button" id="btnRefreshStatus">
                <svg viewBox="0 0 24 24"><path d="M23 4v6h-6M1 20v-6h6"/><path d="M3.51 9a9 9 0 0 1 14.85-3.36L23 10M1 14l4.64 4.36A9 9 0 0 0 20.49 15"/></svg>
                刷新
              </button>
            </div>
            <div class="toolbar__actions hidden" id="toolbarActionsSettings">
              <button class="button primary" id="btnSaveSettings">
                <svg viewBox="0 0 24 24"><path d="M19 21H5a2 2 0 0 1-2-2V5a2 2 0 0 1 2-2h11l5 5v11a2 2 0 0 1-2 2z"/><polyline points="17 21 17 13 7 13 7 21"/><polyline points="7 3 7 8 15 8"/></svg>
                保存设置
              </button>
            </div>
          </header>

          <!-- VIEW: STATUS -->
          <section id="viewStatus">
            <p class="page-subtitle">实时监控后台守护状态、定位坐标与下一次主题切换事件</p>
            <h2 class="section-title">运行概览</h2>
            <div class="status-grid">
              <div class="status-card">
                <div class="status-card-top">
                  <div class="status-card-icon sun" id="solarCardIcon">
                    <svg viewBox="0 0 24 24"><circle cx="12" cy="12" r="5"/><path d="M12 1v2M12 21v2M4.22 4.22l1.42 1.42M18.36 18.36l1.42 1.42M1 12h2M21 12h2M4.22 19.78l1.42-1.42M18.36 5.64l1.42-1.42"/></svg>
                  </div>
                  <div class="status-card-title">太阳相位</div>
                  <span class="status-tag" id="solarTag"><i></i>白天</span>
                </div>
                <div class="status-meta">
                  <div class="status-meta-row"><span>下次日出</span><b id="valNextSunrise">--</b></div>
                  <div class="status-meta-row"><span>下次日落</span><b id="valNextSunset">--</b></div>
                  <div class="status-meta-row"><span id="lblNextSwitch">下次切换</span><b id="valNextSwitch">--</b></div>
                </div>
              </div>

              <div class="status-card">
                <div class="status-card-top">
                  <div class="status-card-icon">
                    <svg viewBox="0 0 24 24"><path d="M21 10c0 7-9 13-9 13s-9-6-9-13a9 9 0 0 1 18 0z"/><circle cx="12" cy="10" r="3"/></svg>
                  </div>
                  <div class="status-card-title">地理定位</div>
                  <span class="status-tag" id="locationTag"><i></i>已定位</span>
                </div>
                <div class="status-meta">
                  <div class="status-meta-row"><span>位置来源</span><b id="valSource">--</b></div>
                  <div class="status-meta-row"><span>当前坐标</span><b id="valCoords">--</b></div>
                  <div class="status-meta-row"><span>上次定位</span><b id="valLastLoc">--</b></div>
                </div>
              </div>

              <div class="status-card">
                <div class="status-card-top">
                  <div class="status-card-icon moon">
                    <svg viewBox="0 0 24 24"><rect x="2" y="3" width="20" height="14" rx="2" ry="2"/><line x1="8" y1="21" x2="16" y2="21"/><line x1="12" y1="17" x2="12" y2="21"/></svg>
                  </div>
                  <div class="status-card-title">Windows 主题</div>
                  <span class="status-tag" id="themeTag"><i></i>正常</span>
                </div>
                <div class="status-meta">
                  <div class="status-meta-row"><span>应用模式</span><b id="valThemeApps">--</b></div>
                  <div class="status-meta-row"><span>系统模式</span><b id="valThemeSystem">--</b></div>
                  <div class="status-meta-row"><span>手动覆盖</span><b id="valOverride">未激活</b></div>
                </div>
              </div>
            </div>

            <h2 class="section-title">实时运行控制台</h2>
            <p class="section-desc">后台守护与定位诊断的轮询输出，每 3 秒自动刷新。</p>
            <div class="setting-section">
              <div class="log-toolbar">
                <span class="log-meta" id="logTimestamp">轮询周期：3 秒</span>
              </div>
              <div class="log-panel" id="logPanel"></div>
            </div>
          </section>

          <!-- VIEW: SETTINGS -->
          <section id="viewSettings" class="hidden">
            <p class="page-subtitle">配置太阳时间计算规则、定位模式与自动切换行为</p>
            <form id="settingsForm" onsubmit="return false;">
              <section class="setting-section">
                <div class="section-head">
                  <h3 class="section-head__title">运行模式</h3>
                  <p class="section-head__desc">控制主题自动切换与开机启动行为。</p>
                </div>
                <label class="check-row">
                  <input type="checkbox" id="cfgAutoEnabled" checked>
                  <span class="check-text">
                    <strong>自动模式</strong>
                    <small>根据日出日落时间自动切换 Windows 应用与系统深浅色主题</small>
                  </span>
                </label>
                <label class="check-row">
                  <input type="checkbox" id="cfgStartupEnabled">
                  <span class="check-text">
                    <strong>登录 Windows 后自动运行</strong>
                    <small>系统开机登录后在后台静默运行，无需管理员权限</small>
                  </span>
                </label>
              </section>

              <section class="setting-section">
                <div class="section-head">
                  <h3 class="section-head__title">定位方式与坐标</h3>
                  <p class="section-head__desc">选择 Windows 自动定位，或手动填写经纬度。</p>
                </div>
                <div class="form-rows">
                  <div class="form-field">
                    <div class="form-field__text">
                      <div class="form-field__title">定位方式</div>
                      <div class="form-field__desc">自动定位无需手动填写经纬度</div>
                    </div>
                    <div class="form-field__control">
                      <select id="cfgLocationMode">
                        <option value="0">Windows 自动定位</option>
                        <option value="1">手动坐标</option>
                      </select>
                    </div>
                  </div>
                  <div class="two-col">
                    <div class="form-field form-field--stack">
                      <div class="form-field__text">
                        <div class="form-field__title">纬度 (-90 ~ 90)</div>
                        <div class="form-field__desc">例如 31.230416</div>
                      </div>
                      <div class="form-field__control">
                        <input type="text" id="cfgLatitude" placeholder="0.000000" autocomplete="off" disabled>
                      </div>
                    </div>
                    <div class="form-field form-field--stack">
                      <div class="form-field__text">
                        <div class="form-field__title">经度 (-180 ~ 180)</div>
                        <div class="form-field__desc">例如 121.473701</div>
                      </div>
                      <div class="form-field__control">
                        <input type="text" id="cfgLongitude" placeholder="0.000000" autocomplete="off" disabled>
                      </div>
                    </div>
                  </div>
                </div>
              </section>

              <section class="setting-section">
                <div class="section-head">
                  <h3 class="section-head__title">日照偏移</h3>
                  <p class="section-head__desc">微调日出日落的切换时机，范围 -120 到 +120 分钟。</p>
                </div>
                <div class="two-col">
                  <div class="form-field form-field--stack">
                    <div class="form-field__text">
                      <div class="form-field__title">日出偏移（分钟）</div>
                      <div class="form-field__desc">正数推迟，负数提前</div>
                    </div>
                    <div class="form-field__control">
                      <input type="number" id="cfgSunriseOffset" min="-120" max="120" value="0">
                    </div>
                  </div>
                  <div class="form-field form-field--stack">
                    <div class="form-field__text">
                      <div class="form-field__title">日落偏移（分钟）</div>
                      <div class="form-field__desc">范围 -120 到 +120</div>
                    </div>
                    <div class="form-field__control">
                      <input type="number" id="cfgSunsetOffset" min="-120" max="120" value="0">
                    </div>
                  </div>
                </div>
                <div class="form-hint">
                  说明：偏移量可微调切换时机。例如设置日落偏移 +30 分钟，系统将在日落 30 分钟后才切换至深色模式。
                </div>
              </section>
            </form>
          </section>
        </div>
      </div>
    </main>
  </div>

  <div class="toast-stack" id="toastStack" aria-live="polite"></div>)raw";

    // Part 7: JavaScript（主题/视图/桥接 + S11 命令面板 + S6 自绘滚动条）
    html += R"raw(
  <script>
    let isEditingCoordinates = false;

    // 主题应用：显式 .dark class（C++ 推送的 theme.apps 为唯一权威；
    // prefers-color-scheme 仅在初始 head 脚本里作回退起点）。
    function applyThemeClass(appsValue) {
      const dark = (appsValue === 0);
      document.documentElement.classList.toggle("dark", dark);
    }

    // S2：最大化联动（C++ 只读通知 {"type":"window","maximized"} → html.is-maximized，
    // CSS 归零窗口圆角与外描边；sessionStorage 兜底下次启动首帧）。
    function applyMaximized(maxed) {
      document.documentElement.classList.toggle("is-maximized", !!maxed);
      try { window.sessionStorage.setItem("geodark-maximized", maxed ? "1" : "0"); } catch (e) {}
    }

    function showToast(msg, type = "info") {
      const stack = document.getElementById("toastStack");
      const toast = document.createElement("div");
      toast.className = `toast ${type}`;
      toast.textContent = msg;
      stack.appendChild(toast);
      setTimeout(() => {
        toast.classList.add("leaving");
        setTimeout(() => toast.remove(), 250);
      }, 3500);
    }

    function sendAction(payload) {
      window.chrome.webview.postMessage(payload);
    }

    const navStatus = document.getElementById("navStatus");
    const navSettings = document.getElementById("navSettings");
    const viewStatus = document.getElementById("viewStatus");
    const viewSettings = document.getElementById("viewSettings");
    const pageTitle = document.getElementById("pageTitle");
    const toolbarActionsStatus = document.getElementById("toolbarActionsStatus");
    const toolbarActionsSettings = document.getElementById("toolbarActionsSettings");

    function switchView(view) {
      if (view === "settings") {
        navSettings.classList.add("active");
        navStatus.classList.remove("active");
        viewSettings.classList.remove("hidden");
        viewStatus.classList.add("hidden");
        toolbarActionsSettings.classList.remove("hidden");
        toolbarActionsStatus.classList.add("hidden");
        pageTitle.textContent = "参数设置";
      } else {
        navStatus.classList.add("active");
        navSettings.classList.remove("active");
        viewStatus.classList.remove("hidden");
        viewSettings.classList.add("hidden");
        toolbarActionsStatus.classList.remove("hidden");
        toolbarActionsSettings.classList.add("hidden");
        pageTitle.textContent = "运行状态";
      }
      if (typeof window.__macScrollbar !== "undefined" && window.__macScrollbar) {
        window.__macScrollbar.sync();
      }
    }

    navStatus.addEventListener("click", () => switchView("status"));
    navSettings.addEventListener("click", () => switchView("settings"));

    document.getElementById("btnWinClose").addEventListener("click", () => {
      sendAction({ action: "window_close" });
    });
    document.getElementById("btnWinMin").addEventListener("click", () => {
      sendAction({ action: "window_min" });
    });
    document.getElementById("btnWinMax").addEventListener("click", () => {
      sendAction({ action: "window_max" });
    });

    // S9③：拖动区扩展——toolbar 与侧栏 chrome/header 标 data-draggable，
    // 触发同一个既有 window_drag 消息（不新增 C++ 动作）。
    // 双击拖拽带/标题区 = 最大化（与 mac 惯例一致）。
    function bindDragSurface(el) {
      if (!el || el.dataset.dragBound === "1") return;
      el.dataset.dragBound = "1";
      el.addEventListener("mousedown", (e) => {
        if (e.button !== 0) return;
        if (e.target.closest("button, input, select, .traffic, a")) return;
        sendAction({ action: "window_drag" });
      });
      el.addEventListener("dblclick", (e) => {
        if (e.target.closest("button, input, select, .traffic, a")) return;
        sendAction({ action: "window_max" });
      });
    }
    document.querySelectorAll("[data-draggable]").forEach(bindDragSurface);

    const modeSelect = document.getElementById("cfgLocationMode");
    const latInput = document.getElementById("cfgLatitude");
    const lonInput = document.getElementById("cfgLongitude");
    const autoCheck = document.getElementById("cfgAutoEnabled");
    const quickRefreshBtn = document.getElementById("btnQuickRefreshLoc");

    function updateCoordinateInputs() {
      const isManual = modeSelect.value === "1";
      latInput.disabled = !isManual;
      lonInput.disabled = !isManual;
      quickRefreshBtn.disabled = isManual || !autoCheck.checked;
    }

    modeSelect.addEventListener("change", updateCoordinateInputs);
    autoCheck.addEventListener("change", updateCoordinateInputs);

    latInput.addEventListener("focus", () => isEditingCoordinates = true);
    latInput.addEventListener("blur", () => isEditingCoordinates = false);
    lonInput.addEventListener("focus", () => isEditingCoordinates = true);
    lonInput.addEventListener("blur", () => isEditingCoordinates = false);

    function saveSettings() {
      const data = {
        auto_enabled: autoCheck.checked,
        location_mode: parseInt(modeSelect.value, 10),
        latitude: latInput.value.trim(),
        longitude: lonInput.value.trim(),
        sunrise_offset: document.getElementById("cfgSunriseOffset").value.trim(),
        sunset_offset: document.getElementById("cfgSunsetOffset").value.trim(),
        startup_enabled: document.getElementById("cfgStartupEnabled").checked
      };
      sendAction({ action: "save", data });
    }

    document.getElementById("btnSaveSettings").addEventListener("click", saveSettings);

    document.getElementById("btnManualLight").addEventListener("click", () => {
      sendAction({ action: "switch_theme", light: true });
    });

    document.getElementById("btnManualDark").addEventListener("click", () => {
      sendAction({ action: "switch_theme", light: false });
    });

    document.getElementById("btnRefreshStatus").addEventListener("click", () => {
      sendAction({ action: "refresh_status" });
    });

    document.getElementById("btnRefreshEnv").addEventListener("click", () => {
      sendAction({ action: "refresh_status" });
    });

    document.getElementById("btnQuickRefreshLoc").addEventListener("click", () => {
      sendAction({ action: "refresh_location" });
    });

    document.getElementById("btnQuickAuth").addEventListener("click", () => {
      sendAction({ action: "authorize" });
    });

    window.chrome.webview.addEventListener("message", (event) => {
      const msg = event.data;
      if (!msg) return;

      if (msg.toast) {
        showToast(msg.toast.text, msg.toast.type || "info");
      }

      if (msg.switch_view) {
        switchView(msg.switch_view);
      }

      if (msg.theme) {
        applyThemeClass(msg.theme.apps);
      }

      // S9②：最大化状态只读通知（不进 action 分发，字段独立）。
      if (msg.window && typeof msg.window.maximized === "boolean") {
        applyMaximized(msg.window.maximized);
      }

      if (msg.state) {
        renderState(msg.state, msg.config, msg.theme, msg.startup);
      }
    });

    function formatTime(str) {
      return str || "未取得";
    }

    function renderState(state, config, theme, startup) {
      if (theme) applyThemeClass(theme.apps);

      const envPill = document.getElementById("envPill");
      const envPillText = document.getElementById("envPillText");
      if (state.core_running) {
        envPill.classList.add("ok");
        envPillText.textContent = "后台：运行中";
      } else {
        envPill.classList.remove("ok");
        envPillText.textContent = "后台：未运行";
      }

      const envValLoc = document.getElementById("envValLocation");
      const envDotLoc = document.getElementById("envDotLocation");
      if (config.location_mode === 1) {
        envValLoc.textContent = "手动坐标";
        envDotLoc.className = config.has_manual_coordinates ? "ok" : "warn";
      } else {
        envValLoc.textContent = state.has_location ? "已取得" : "未取得";
        envDotLoc.className = state.has_location ? "ok" : "warn";
      }

      const envValSolar = document.getElementById("envValSolar");
      const envDotSolar = document.getElementById("envDotSolar");
      if (state.active_source === 0) {
        envValSolar.textContent = "未知";
        envDotSolar.className = "warn";
      } else {
        envValSolar.textContent = state.expected_light ? "白天" : "夜晚";
        envDotSolar.className = state.polar ? "warn" : "ok";
      }

      const envValTheme = document.getElementById("envValTheme");
      const appThemeStr = theme.apps === 1 ? "浅色" : theme.apps === 0 ? "深色" : "未知";
      const sysThemeStr = theme.system === 1 ? "浅色" : theme.system === 0 ? "深色" : "未知";
      envValTheme.textContent = `${appThemeStr} / ${sysThemeStr}`;

      const solarTag = document.getElementById("solarTag");
      const solarCardIcon = document.getElementById("solarCardIcon");
      if (state.active_source === 0) {
        solarTag.className = "status-tag warn";
        solarTag.innerHTML = "<i></i>未知";
        solarCardIcon.className = "status-card-icon";
      } else if (state.expected_light) {
        solarTag.className = "status-tag";
        solarTag.innerHTML = "<i></i>白天" + (state.polar ? " (极昼)" : "");
        solarCardIcon.className = "status-card-icon sun";
      } else {
        solarTag.className = "status-tag";
        solarTag.innerHTML = "<i></i>夜晚" + (state.polar ? " (极夜)" : "");
        solarCardIcon.className = "status-card-icon moon";
      }

      document.getElementById("valNextSunrise").textContent = formatTime(state.next_sunrise);
      document.getElementById("valNextSunset").textContent = formatTime(state.next_sunset);
      document.getElementById("lblNextSwitch").textContent = config.auto_enabled ? "下次切换" : "下次太阳变化";
      document.getElementById("valNextSwitch").textContent = formatTime(state.next_switch);

      const locationTag = document.getElementById("locationTag");
      const hasCoords = (config.location_mode === 1) ? config.has_manual_coordinates : state.has_location;
      if (hasCoords) {
        locationTag.className = "status-tag";
        locationTag.innerHTML = "<i></i>已就绪";
      } else {
        locationTag.className = "status-tag warn";
        locationTag.innerHTML = "<i></i>等待定位";
      }

      document.getElementById("valSource").textContent = state.source_name;
      if (state.active_source !== 0) {
        const lat = config.location_mode === 1 ? config.manual_latitude : state.latitude;
        const lon = config.location_mode === 1 ? config.manual_longitude : state.longitude;
        document.getElementById("valCoords").textContent = `${lat.toFixed(4)}, ${lon.toFixed(4)}`;
      } else {
        document.getElementById("valCoords").textContent = "--";
      }

      if (config.location_mode === 0) {
        let locTime = formatTime(state.location_time);
        if (state.has_location && state.accuracy_meters > 0) {
          locTime += ` (约 ${Math.round(state.accuracy_meters)} 米)`;
        }
        document.getElementById("valLastLoc").textContent = locTime;
      } else {
        document.getElementById("valLastLoc").textContent = "手动输入";
      }

      const themeTag = document.getElementById("themeTag");
      themeTag.className = "status-tag";
      themeTag.innerHTML = `<i></i>${appThemeStr}`;
      document.getElementById("valThemeApps").textContent = appThemeStr;
      document.getElementById("valThemeSystem").textContent = sysThemeStr;
      if (state.override_active) {
        const until = state.override_until ? state.override_until : "下一次太阳事件";
        document.getElementById("valOverride").textContent = `覆盖至 ${until}`;
      } else {
        document.getElementById("valOverride").textContent = "未激活";
      }

      if (!isEditingCoordinates) {
        modeSelect.value = config.location_mode.toString();
        if (config.has_manual_coordinates) {
          latInput.value = config.manual_latitude.toFixed(6);
          lonInput.value = config.manual_longitude.toFixed(6);
        }
        autoCheck.checked = config.auto_enabled;
        document.getElementById("cfgStartupEnabled").checked = startup;
        document.getElementById("cfgSunriseOffset").value = config.sunrise_offset;
        document.getElementById("cfgSunsetOffset").value = config.sunset_offset;
        updateCoordinateInputs();
      }

      renderLogPanel(state, config, theme);
    }

    function renderLogPanel(state, config, theme) {
      const panel = document.getElementById("logPanel");
      const now = new Date();
      const timeStr = now.toTimeString().split(' ')[0];

      let lines = [];
      lines.push({ cls: "cmd", text: `[GeoDark] 运行参数检查 (${timeStr})` });
      lines.push({ cls: state.core_running ? "ok" : "err", text: `后台进程: ${state.core_running ? "运行中 (已就绪)" : "未运行"}` });
      lines.push({ cls: "info", text: `自动模式: ${config.auto_enabled ? "开启" : "关闭"} | 定位模式: ${config.location_mode === 1 ? "手动坐标" : "Windows 自动定位"}` });
      lines.push({ cls: "info", text: `位置来源: ${state.source_name}` });
      if (state.active_source !== 0) {
        const lat = config.location_mode === 1 ? config.manual_latitude : state.latitude;
        const lon = config.location_mode === 1 ? config.manual_longitude : state.longitude;
        lines.push({ cls: "info", text: `当前经纬度: 纬度 ${lat.toFixed(6)}, 经度 ${lon.toFixed(6)}` });
      }
      lines.push({ cls: "info", text: `下次日出: ${formatTime(state.next_sunrise)} | 下次日落: ${formatTime(state.next_sunset)}` });
      lines.push({ cls: "info", text: `下次切换目标: ${formatTime(state.next_switch)}` });
      lines.push({ cls: "info", text: `Windows 主题: 应用 ${theme.apps === 1 ? "浅色" : "深色"} / 系统 ${theme.system === 1 ? "浅色" : "深色"}` });
      if (state.override_active) {
        lines.push({ cls: "warn", text: `手动临时覆盖: 生效至 ${state.override_until || "下一次太阳事件"}` });
      }
      if (state.location_error) {
        lines.push({ cls: "err", text: `定位诊断信息: ${state.location_error}` });
      }
      if (state.theme_error) {
        lines.push({ cls: "err", text: `主题设置错误: ${state.theme_error}` });
      }

      panel.innerHTML = lines.map(l =>
        `<div class="log-line ${l.cls}"><time>${timeStr}</time><span>${escapeHTML(l.text)}</span></div>`
      ).join("");
    }

    function escapeHTML(str) {
      return (str || "").replace(/[&<>"']/g, m => ({
        "&": "&amp;", "<": "&lt;", ">": "&gt;", '"': "&quot;", "'": "&#39;"
      })[m]);
    }
)raw";

    // Part 8: S11 命令面板 + S6 自绘滚动条（vanilla 内嵌，动作复用既有 action）
    html += R"raw(
    /* ================= S11 命令面板 =================
       ⌘K / Ctrl+K、侧栏搜索钮呼出；Esc 关闭、↑↓ 选择、Enter 执行。
       动作全部复用既有 action（视图跳转 / refresh_status / refresh_location /
       authorize / switch_theme / save），不发明新 action。 */
    const commandPalette = (function () {
      let mask = null, input = null, list = null;
      let activeIndex = 0;
      let filtered = [];

      const commands = [
        {
          id: "view-status", label: "前往运行状态", group: "视图", hint: "运行概览与控制台",
          icon: '<circle cx="12" cy="12" r="4"/><path d="M12 2v2M12 20v2M4.93 4.93l1.41 1.41M17.66 17.66l1.41 1.41M2 12h2M20 12h2M6.34 17.66l-1.41 1.41M19.07 4.93l-1.41 1.41"/>',
          run: () => switchView("status")
        },
        {
          id: "view-settings", label: "前往参数设置", group: "视图", hint: "打开偏好设置",
          icon: '<circle cx="12" cy="12" r="3"/><path d="M19.4 15a1.65 1.65 0 0 0 .33 1.82l.06.06a2 2 0 0 1 0 2.83 2 2 0 0 1-2.83 0l-.06-.06a1.65 1.65 0 0 0-1.82-.33 1.65 1.65 0 0 0-1 1.51V21a2 2 0 0 1-2 2 2 2 0 0 1-2-2v-.09A1.65 1.65 0 0 0 9 19.4a1.65 1.65 0 0 0-1.82.33l-.06.06a2 2 0 0 1-2.83 0 2 2 0 0 1 0-2.83l.06-.06a1.65 1.65 0 0 0 .33-1.82 1.65 1.65 0 0 0-1.51-1H3a2 2 0 0 1-2-2 2 2 0 0 1 2-2h.09A1.65 1.65 0 0 0 4.6 9a1.65 1.65 0 0 0-.33-1.82l-.06-.06a2 2 0 0 1 0-2.83 2 2 0 0 1 2.83 0l.06.06a1.65 1.65 0 0 0 1.82.33H9a1.65 1.65 0 0 0 1-1.51V3a2 2 0 0 1 2-2 2 2 0 0 1 2 2v.09a1.65 1.65 0 0 0 1 1.51 1.65 1.65 0 0 0 1.82-.33l.06-.06a2 2 0 0 1 2.83 0 2 2 0 0 1 0 2.83l-.06.06a1.65 1.65 0 0 0-.33 1.82V9a1.65 1.65 0 0 0 1.51 1H21a2 2 0 0 1 2 2 2 2 0 0 1-2 2h-.09a1.65 1.65 0 0 0-1.51 1z"/>',
          run: () => switchView("settings")
        },
        {
          id: "refresh-status", label: "立即刷新状态", group: "操作", hint: "重新读取后台与定位",
          icon: '<path d="M23 4v6h-6M1 20v-6h6"/><path d="M3.51 9a9 9 0 0 1 14.85-3.36L23 10M1 14l4.64 4.36A9 9 0 0 0 20.49 15"/>',
          run: () => sendAction({ action: "refresh_status" })
        },
        {
          id: "refresh-location", label: "刷新定位", group: "操作", hint: "请求后台重新定位",
          icon: '<path d="M21.5 2v6h-6M21.34 15.57a10 10 0 1 1-.57-8.38l5.67-5.67"/>',
          run: () => sendAction({ action: "refresh_location" })
        },
        {
          id: "authorize-location", label: "打开定位授权", group: "操作", hint: "Windows 位置权限",
          icon: '<path d="M12 22s8-4 8-10V5l-8-3-8 3v7c0 6 8 10 8 10z"/>',
          run: () => sendAction({ action: "authorize" })
        },
        {
          id: "save-settings", label: "保存当前设置", group: "操作", hint: "写入参数并重启后台",
          icon: '<path d="M19 21H5a2 2 0 0 1-2-2V5a2 2 0 0 1 2-2h11l5 5v11a2 2 0 0 1-2 2z"/><polyline points="17 21 17 13 7 13 7 21"/><polyline points="7 3 7 8 15 8"/>',
          run: () => saveSettings()
        },
        {
          id: "theme-light", label: "切换为浅色主题", group: "主题", hint: "立即生效",
          icon: '<circle cx="12" cy="12" r="5"/><path d="M12 1v2M12 21v2M4.22 4.22l1.42 1.42M18.36 18.36l1.42 1.42M1 12h2M21 12h2M4.22 19.78l1.42-1.42M18.36 5.64l1.42-1.42"/>',
          run: () => sendAction({ action: "switch_theme", light: true })
        },
        {
          id: "theme-dark", label: "切换为深色主题", group: "主题", hint: "立即生效",
          icon: '<path d="M21 12.79A9 9 0 1 1 11.21 3 7 7 0 0 0 21 12.79z"/>',
          run: () => sendAction({ action: "switch_theme", light: false })
        }
      ];

      function matches(cmd, q) {
        if (!q) return true;
        const hay = ((cmd.label || "") + " " + (cmd.hint || "") + " " + (cmd.group || ""))
          .toLowerCase();
        return hay.indexOf(q) >= 0;
      }

      function build() {
        if (mask) return;
        mask = document.createElement("div");
        mask.className = "cmdk-mask";
        mask.innerHTML =
          '<div class="cmdk" role="dialog" aria-label="命令面板">' +
          '  <div class="cmdk-input-wrap">' +
          '    <svg viewBox="0 0 24 24"><circle cx="11" cy="11" r="7"/><path d="M20 20l-3.5-3.5"/></svg>' +
          '    <input type="text" class="cmdk-input" aria-label="搜索命令" placeholder="跳转视图、刷新状态、切换主题…" />' +
          '    <span class="cmdk-esc">Esc</span>' +
          '  </div>' +
          '  <div class="cmdk-list"></div>' +
          '</div>';
        document.body.appendChild(mask);
        input = mask.querySelector(".cmdk-input");
        list = mask.querySelector(".cmdk-list");

        mask.addEventListener("mousedown", (e) => {
          if (e.target === mask) close();
        });
        input.addEventListener("input", () => paint());
        input.addEventListener("keydown", (e) => {
          if (e.key === "ArrowDown") { e.preventDefault(); move(1); }
          else if (e.key === "ArrowUp") { e.preventDefault(); move(-1); }
          else if (e.key === "Enter") {
            e.preventDefault();
            const cmd = filtered[activeIndex];
            if (cmd) exec(cmd);
          }
        });
      }

      function paint() {
        const q = (input.value || "").trim().toLowerCase();
        filtered = commands.filter((c) => matches(c, q));
        if (activeIndex >= filtered.length) activeIndex = 0;

        if (!filtered.length) {
          list.innerHTML = '<div class="cmdk-empty">没有匹配的命令。</div>';
          return;
        }

        let lastGroup = null;
        let htmlOut = "";
        filtered.forEach((cmd, idx) => {
          const group = cmd.group || "其他";
          if (group !== lastGroup) {
            lastGroup = group;
            htmlOut += '<div class="cmdk-group-heading">' + escapeHTML(group) + '</div>';
          }
          htmlOut +=
            '<div class="cmdk-item' + (idx === activeIndex ? " active" : "") + '" data-idx="' + idx + '">' +
            '<svg viewBox="0 0 24 24">' + cmd.icon + '</svg>' +
            '<span class="cmdk-item-label">' + escapeHTML(cmd.label) + '</span>' +
            (cmd.hint ? '<span class="cmdk-item-hint">' + escapeHTML(cmd.hint) + '</span>' : '') +
            '</div>';
        });
        list.innerHTML = htmlOut;

        list.querySelectorAll(".cmdk-item").forEach((el) => {
          el.addEventListener("click", () => {
            const cmd = filtered[parseInt(el.getAttribute("data-idx"), 10)];
            if (cmd) exec(cmd);
          });
          el.addEventListener("mousemove", () => {
            const idx = parseInt(el.getAttribute("data-idx"), 10);
            if (idx !== activeIndex) { activeIndex = idx; mark(); }
          });
        });

        scrollActiveIntoView();
      }

      function mark() {
        list.querySelectorAll(".cmdk-item").forEach((el) => {
          el.classList.toggle("active", parseInt(el.getAttribute("data-idx"), 10) === activeIndex);
        });
        scrollActiveIntoView();
      }

      function scrollActiveIntoView() {
        const el = list.querySelector(".cmdk-item.active");
        if (el && el.scrollIntoView) el.scrollIntoView({ block: "nearest" });
      }

      function move(delta) {
        if (!filtered.length) return;
        activeIndex = (activeIndex + delta + filtered.length) % filtered.length;
        mark();
      }

      function open() {
        build();
        mask.classList.add("open");
        input.value = "";
        activeIndex = 0;
        paint();
        setTimeout(() => input.focus(), 0);
      }

      function close() {
        if (!mask) return;
        mask.classList.remove("open");
        input.value = "";
      }

      function isOpen() { return !!(mask && mask.classList.contains("open")); }

      function exec(cmd) {
        close();
        try { cmd.run(); } catch (e) {}
      }

      document.addEventListener("keydown", (e) => {
        if ((e.metaKey || e.ctrlKey) && String(e.key || "").toLowerCase() === "k") {
          e.preventDefault();
          if (isOpen()) close(); else open();
          return;
        }
        if (e.key === "Escape" && isOpen()) {
          e.preventDefault();
          close();
        }
      });

      return { open, close, isOpen };
    })();

    document.getElementById("btnOpenPalette").addEventListener("click", () => {
      commandPalette.open();
    });

    /* ================= S6 自绘悬浮滚动条 =================
       移植自模板 scrollbar.tsx（PortPilot mac-scrollbar.js 同款）：
       透明轨道 + 胶囊滑块，rAF 合帧同步，1:1 拖拽与轨道点击跳转。 */
    window.__macScrollbar = (function () {
      const scroller = document.getElementById("pageScroller");
      if (!scroller) return null;

      const track = document.createElement("div");
      track.className = "mac-scrollbar";
      track.setAttribute("aria-hidden", "true");
      const thumb = document.createElement("div");
      thumb.className = "mac-scrollbar__thumb";
      track.appendChild(thumb);
      document.body.appendChild(track);

      let raf = 0;
      let drag = null;
      let on = false;

      function visible(next) {
        if (on === next) return;
        on = next;
        track.classList.toggle("is-on", on);
      }

      function sync() {
        const view = scroller.clientHeight;
        const total = scroller.scrollHeight;
        if (total <= view + 1) { visible(false); return; }
        visible(true);

        const trackH = track.clientHeight;
        if (trackH <= 0) return;

        const thumbH = Math.max(32, Math.round((trackH * view) / total));
        const maxScroll = Math.max(1, total - view);
        const offset = (scroller.scrollTop / maxScroll) * Math.max(0, trackH - thumbH);

        thumb.style.height = thumbH + "px";
        thumb.style.transform = "translateY(" + offset + "px)";
      }

      function scheduleSync() {
        if (raf) window.cancelAnimationFrame(raf);
        raf = window.requestAnimationFrame(sync);
      }

      thumb.addEventListener("pointerdown", (e) => {
        e.preventDefault();
        e.stopPropagation();
        try { thumb.setPointerCapture(e.pointerId); } catch (err) {}
        drag = { startY: e.clientY, startScroll: scroller.scrollTop };
        track.classList.add("is-dragging");
      });

      thumb.addEventListener("pointermove", (e) => {
        if (!drag) return;
        const span = Math.max(1, track.clientHeight - thumb.offsetHeight);
        const maxScroll = Math.max(0, scroller.scrollHeight - scroller.clientHeight);
        scroller.scrollTop = drag.startScroll + (e.clientY - drag.startY) * (maxScroll / span);
      });

      function endDrag(e) {
        if (!drag) return;
        drag = null;
        track.classList.remove("is-dragging");
        if (e && e.pointerId !== undefined) {
          try {
            if (thumb.hasPointerCapture(e.pointerId)) thumb.releasePointerCapture(e.pointerId);
          } catch (err) {}
        }
      }
      thumb.addEventListener("pointerup", endDrag);
      thumb.addEventListener("pointercancel", endDrag);

      track.addEventListener("click", (e) => {
        if (e.target === thumb || drag) return;
        const rect = track.getBoundingClientRect();
        const clickY = e.clientY - rect.top;
        const thumbH = thumb.offsetHeight;
        const span = Math.max(1, track.clientHeight - thumbH);
        const maxScroll = Math.max(0, scroller.scrollHeight - scroller.clientHeight);
        const ratio = Math.max(0, Math.min(1, (clickY - thumbH / 2) / span));
        scroller.scrollTo({ top: ratio * maxScroll, behavior: "smooth" });
      });

      scroller.addEventListener("scroll", sync, { passive: true });
      window.addEventListener("resize", scheduleSync);

      let ro = null;
      if (typeof ResizeObserver !== "undefined") {
        ro = new ResizeObserver(scheduleSync);
        ro.observe(scroller);
        const inner = scroller.firstElementChild;
        if (inner) ro.observe(inner);
      }

      scheduleSync();
      setTimeout(sync, 250);

      return { sync };
    })();

    window.chrome.webview.postMessage({ action: "init" });
  </script>
</body>
</html>)raw";

    return html;
}

} // namespace geodark
