#pragma once

#include <string>

namespace geodark {

// 本文件为单 HTML 内嵌 UI（HTML+CSS+JS raw string），经 WebView2 NavigateToString
// 加载。材质 token 来源 macUI-SolidGlass 模板本机副本，复制日期 2026-11-22。
// 桥协议不变：JS→C++ postMessage({action})；C++→JS PostWebMessageAsJson
// （state/config/theme/toast/switch_view）。
inline std::string get_ui_html() {
    std::string html;
    html.reserve(32768);

    // Part 1: Head & CSS styles
    html += R"raw(<!DOCTYPE html>
<html lang="zh-CN">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>GeoDark 设置</title>
  <script>
    // 主题初始回退：仅当宿主未推送 theme 消息时以 prefers-color-scheme 定起点；
    // 之后一律以 C++ 推送的 theme.apps 为准（applyThemeClass）。
    (function () {
      var dark = false;
      try { dark = window.matchMedia("(prefers-color-scheme: dark)").matches; } catch (e) { dark = false; }
      if (dark) document.documentElement.classList.add("dark");
    })();
  </script>
  <style>
    /* ====================================================================
       Solid Glass —— GeoDark 内嵌 UI 材质
       （材质 token 来源 macUI-SolidGlass 模板本机副本，复制日期 2026-11-22）

       Solid Glass 介于平面设计与液态玻璃之间：每一块面都是一块厚实的烟玻璃，
       透出极少——远看是实心，近看才是玻璃。让它是玻璃的，全在边缘：
       顶上一线光（rim）、四周一根发丝线（hairline）、底下收拢的阴影（shadow）。
       面本身是中性灰：不取背后之色，不折光，不流动。

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
      font-family: Inter, "SF Pro Display", "Segoe UI Variable Display", "Segoe UI", system-ui, -apple-system, "PingFang SC", "Hiragino Sans GB", "Microsoft YaHei", sans-serif;
      font-optical-sizing: auto;
      color: var(--foreground);
      background: transparent;
      font-synthesis: none;
      text-size-adjust: 100%;
      -webkit-text-size-adjust: 100%;
      -webkit-font-smoothing: antialiased;
      -moz-osx-font-smoothing: grayscale;
      text-rendering: optimizeLegibility;

      /* ---- 面板几何 ---- */
      --radius: 12px;
      --radius-control: 9px;

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
      --blue: oklch(0.55 0.19 257);
      --blue-dark: oklch(0.47 0.17 257);

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
      --blue: oklch(0.76 0.12 250);
      --blue-dark: oklch(0.72 0.14 250);

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

    /* 兼容旧内联样式里的硬编码深浅色覆盖：由 token 接管，不再需要。
       )raw";

    html += R"raw(（保留选择器本身为空会拖慢解析，直接删除。） */

    * { box-sizing: border-box; user-select: none; }
    ::-webkit-scrollbar { width: 7px; height: 7px; }
    ::-webkit-scrollbar-track { background: transparent; }
    ::-webkit-scrollbar-thumb { background: color-mix(in oklch, var(--foreground) 26%, transparent); border-radius: 4px; }
    ::-webkit-scrollbar-thumb:hover { background: color-mix(in oklch, var(--foreground) 44%, transparent); }
    html, body, #app {
      margin: 0; width: 100%; height: 100%; overflow: hidden;
    }
    body {
      background-color: var(--background);
      background-image:
        radial-gradient(62rem 42rem at 12% -6rem, var(--light-1), transparent 62%),
        radial-gradient(52rem 38rem at 92% 4rem, var(--light-2), transparent 60%),
        radial-gradient(64rem 46rem at 55% 110%, var(--light-3), transparent 62%);
      background-repeat: no-repeat;
      background-attachment: fixed;
    }
    button, input, select { font: inherit; }
    button { color: inherit; }
    svg {
      width: 18px; height: 18px; fill: none; stroke: currentColor;
      stroke-width: 1.8; stroke-linecap: round; stroke-linejoin: round;
      shape-rendering: geometricPrecision;
    }

    /* ---- 材质底座 -------------------------------------------------------
       窗壳让出背景：环境光铺在 body 上，玻璃面直接立于其上。 */
    .window-shell {
      height: 100%; display: flex; flex-direction: row; background: transparent; overflow: hidden;
    }
    .main-area {
      position: relative; min-width: 0; min-height: 0; flex: 1; display: flex; flex-direction: column; overflow: hidden;
    }

    /* 侧栏：立在边缘上的一块板，光落在面向页面的一侧。
       「面板」厚度档（最薄）。 */
    .sidebar {
      width: 224px; height: 100%; flex: 0 0 224px; padding: 0 13px 16px;
      background: var(--sidebar); display: flex; flex-direction: column;
      -webkit-backdrop-filter: var(--glass-blur); backdrop-filter: var(--glass-blur);
      box-shadow:
        inset -1px 0 0 var(--glass-edge),
        inset 0 1px 0 var(--glass-inner);
    }
    .sidebar-chrome {
      height: 52px; flex: 0 0 52px; margin: 0 -13px; padding: 0 16px; display: flex; align-items: center; cursor: default;
    }

    /* 红黄绿窗控：三颗小灯，最小的实心玻璃件（行为消息原样保留）。 */
    .traffic { display: flex; gap: 8px; align-items: center; }
    .light {
      position: relative; width: 13px; height: 13px; display: grid; place-items: center;
      border-radius: 50%; border: 0.5px solid rgba(0, 0, 0, 0.12); padding: 0; cursor: pointer;
      transition: filter 0.12s ease, transform 0.1s ease;
    }
    .light.close { background: #ff5f57; }
    .light.min { background: #febc2e; }
    .light.max { background: #28c840; }
    .light::before {
      font: 700 10px/1 "Segoe UI", sans-serif; opacity: 0; transform: translateY(-0.4px); transition: opacity 0.1s ease;
    }
    .light.close::before { content: "×"; color: rgba(88, 0, 0, 0.66); }
    .light.min::before { content: "−"; color: rgba(93, 54, 0, 0.68); }
    .light.max::before {
      content: ""; width: 8px; height: 8px;
      background: center/contain no-repeat url("data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 10 10'%3E%3Cpath d='M1.7 4.4V1.7h2.7M4.4 1.7 1.7 4.4M8.3 5.6v2.7H5.6M5.6 8.3l2.7-2.7' fill='none' stroke='%23075b16' stroke-width='1.25' stroke-linecap='round' stroke-linejoin='round'/%3E%3C/svg%3E");
    }
    .traffic:hover .light::before { opacity: 1; }
    .light:hover { filter: brightness(0.96); }
    .light:active { filter: brightness(0.88); transform: scale(0.9); }

    .sidebar-identity { display: grid; gap: 10px; padding: 2px 10px 22px; cursor: default; }
    .title { display: flex; align-items: center; gap: 9px; font-size: 14px; font-weight: 650; letter-spacing: -0.01em; }

    /* 品牌墨块：主按钮那块实心板，缩到一枚图标的大小。 */
    .title-mark {
      display: flex; align-items: center; justify-content: center; width: 24px; height: 24px; border-radius: 28%;
      background: linear-gradient(135deg, oklch(0.75 0.15 75), oklch(0.55 0.19 257)); color: white;
      box-shadow:
        inset 0 1px 0 var(--primary-rim),
        inset 0 -1px 0 var(--primary-under),
        0 1px 2px var(--glass-contact);
    }
    .title-mark svg { width: 15px; height: 15px; stroke-width: 2.2; }

    /* 环境 pill：一块透明玻璃小件。 */
    .env-pill {
      width: 100%; min-width: 0; display: flex; align-items: center; gap: 7px; padding: 5px 8px; border-radius: 7px;
      background: var(--glass-control); color: var(--muted); font-size: 11px; font-weight: 600;
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

    .sidebar-label {
      margin: 0 10px 8px; color: var(--muted); font-size: 11px; font-weight: 650; text-transform: uppercase; letter-spacing: 0.04em;
    }
    .settings-label { margin-top: 25px; }

    /* 侧栏导航项：唯一状态 glass-state 的主场。 */
    .nav {
      width: 100%; border: 0; background: transparent; border-radius: var(--radius-control); display: flex; align-items: center; gap: 10px;
      padding: 8px 10px; margin: 2px 0; font-size: 13px; font-weight: 520; cursor: pointer; text-align: left;
      transition: background-color 0.15s, transform 0.1s;
    }
    .nav:hover { background: var(--hover); }
    .nav:active { transform: scale(0.985); }
    .nav.active {
      background: var(--glass-state);
      color: var(--blue); font-weight: 600;
      box-shadow:
        inset 0 1px 0 var(--glass-state-rim),
        0 0 0 1px var(--glass-state-edge);
    }
    .nav svg { width: 17px; height: 17px; }
    .nav span { flex: 1; }

    /* 环境卡片：立在同一块侧栏上的薄板（panel 档，共享其模糊）。 */
    .environment-card {
      margin-top: auto; padding: 12px; border: 0; border-radius: 10px;
      background: var(--panel); font-size: 11px;
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
    .environment-card b { margin-left: auto; font-size: 10px; font-weight: 550; color: var(--foreground); }

    .card-actions { display: grid; grid-template-columns: 1fr 1fr; gap: 6px; margin-top: 10px; }

    /* 卡片小按钮：透明玻璃（outline 按钮的处理）。 */
    .card-action-btn {
      width: 100%; border: 1px solid var(--border); border-radius: 6px; padding: 6px; color: var(--foreground);
      background: var(--glass-control); font-size: 10px; font-weight: 600; cursor: pointer;
      display: inline-flex; align-items: center; justify-content: center; gap: 4px;
      transition: background-color 0.15s;
    }
    .card-action-btn:hover:not(:disabled) { background: var(--hover); }
    .card-action-btn:disabled { opacity: 0.45; cursor: default; }
)raw";

    html += R"raw(
    .content { min-width: 0; min-height: 0; flex: 1; overflow: hidden; background: transparent; }
    .view { height: 100%; overflow-y: auto; padding: 30px clamp(24px, 4vw, 50px) 40px; }
    .view.hidden { display: none !important; }
    .page-head { display: flex; align-items: flex-start; justify-content: space-between; gap: 20px; margin-bottom: 24px; }
    h1 { margin: 0 0 5px; font-size: 26px; letter-spacing: -0.035em; font-weight: 720; }
    .page-head p { margin: 0; color: var(--muted); font-size: 13px; }
    .head-actions { display: flex; gap: 8px; }

    /* 普通按钮：透明玻璃，发丝线是它自己的边框。 */
    .button {
      height: 34px; padding: 0 14px; border-radius: var(--radius-control); border: 1px solid var(--border);
      background: var(--glass-control); color: var(--foreground);
      display: inline-flex; align-items: center; justify-content: center; gap: 6px; font-size: 12px; font-weight: 620; cursor: pointer;
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

    /* 设置分节 / 状态卡：card 档（中间厚度）。边框让位，收光三件套接管。 */
    .setting-section {
      padding: 20px 22px; margin-bottom: 14px; background: var(--card); border: 0; border-radius: var(--radius);
      -webkit-backdrop-filter: var(--glass-blur); backdrop-filter: var(--glass-blur);
      box-shadow:
        inset 0 1px 0 var(--glass-rim),
        inset 0 0 0 1px var(--glass-inner),
        0 0 0 1px var(--glass-edge),
        0 1px 2px var(--glass-contact),
        0 12px 32px -14px var(--glass-shade);
    }
    .setting-section h3 { margin: 0 0 16px; font-size: 13px; font-weight: 650; letter-spacing: -0.01em; }
    label { display: block; color: color-mix(in oklch, var(--foreground) 88%, transparent); font-size: 12px; font-weight: 600; }
    label > span { float: right; color: var(--muted); font-size: 10px; font-weight: 400; }

    /* 输入域：一块可敲字的透明玻璃——rim 在顶缘内侧，发丝线是它自己的边框，
       聚焦时边框变成更亮的一根线，仅此而已。 */
    input, select {
      width: 100%; height: 36px; margin-top: 7px; padding: 0 11px; outline: 0;
      border: 1px solid var(--field-edge); border-radius: var(--radius-control); background: var(--field);
      color: inherit; font-size: 12px;
      transition: border-color 0.15s, background-color 0.15s; user-select: text;
    }
    input:focus, select:focus {
      border-color: var(--focus-line); background: var(--field);
    }
    input:disabled, select:disabled { opacity: 0.45; background: var(--secondary); cursor: not-allowed; }
    .two-col { display: grid; grid-template-columns: 1fr 1fr; gap: 14px; }
    .setting-section label + label, .setting-section .two-col + label, .setting-section label + .two-col, .setting-section .check-row + .check-row { margin-top: 14px; }

    /* 开关行：立着的最薄板（panel 档）；选中行穿唯一状态的玻璃。 */
    .check-row {
      display: flex; align-items: center; gap: 12px; padding: 12px; border: 0;
      border-radius: 10px; background: var(--panel); cursor: pointer;
      box-shadow:
        inset 0 1px 0 var(--glass-inner),
        inset 0 0 0 1px var(--glass-edge);
    }
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
    .check-row span { float: none; flex: 1; }
    .check-row strong { display: block; font-size: 12px; font-weight: 600; }
    .check-row small { display: block; margin-top: 2px; color: var(--muted); font-size: 10px; font-weight: 400; }
    .form-hint { margin-top: 10px; color: var(--muted); font-size: 11px; }
    .status-grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(240px, 1fr)); gap: 14px; margin-bottom: 20px; }

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
    .status-card-title { flex: 1; font-size: 13px; font-weight: 650; }
    .status-tag {
      padding: 3px 8px; border-radius: 99px; font-size: 10px; font-weight: 650;
      background: color-mix(in oklch, var(--success) 12%, transparent); color: var(--success);
      display: inline-flex; align-items: center; gap: 4px;
    }
    .status-tag i { width: 6px; height: 6px; border-radius: 50%; background: currentColor; }
    .status-tag.warn { background: color-mix(in oklch, var(--warn) 14%, transparent); color: var(--warn); }
    .status-tag.bad { background: color-mix(in oklch, var(--danger) 14%, transparent); color: var(--danger); }
    .status-meta { display: grid; gap: 6px; font-size: 11px; }
    .status-meta-row { display: flex; justify-content: space-between; color: var(--muted); }
    .status-meta-row b { color: var(--foreground); font-weight: 550; }

    /* 日志面板：厚板（popover 档）上挖出的墨色凹井——凹槽规则，
       光从反向落；文字颜色不变。 */
    .log-panel {
      background: var(--log-bg); border-radius: 10px; padding: 14px 0; overflow-y: auto; max-height: 320px; min-height: 200px;
      color: var(--log-fg);
      box-shadow:
        inset 0 1px 1.5px var(--well-shade),
        inset 0 -1px 0 var(--well-rim),
        inset 0 0 0 1px var(--glass-edge);
    }
    .log-line {
      display: flex; gap: 12px; padding: 4px 16px; font: 11px/1.5 "Cascadia Mono", Consolas, monospace; user-select: text;
    }
    .log-line time { flex: 0 0 65px; color: var(--log-dim); }
    .log-line span { white-space: pre-wrap; overflow-wrap: anywhere; }
    .log-line.cmd span { color: oklch(0.72 0.12 250); }
    .log-line.ok span { color: var(--success); }
    .log-line.warn span { color: var(--warn); }
    .log-line.err span { color: var(--danger); }
)raw";

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
      color: var(--foreground); font-size: 12px; font-weight: 500;
      animation: toastIn 0.2s ease-out; transition: opacity 0.2s, transform 0.2s;
    }
    .toast.success { border-left-color: var(--success); }
    .toast.error { border-left-color: var(--danger); }
    .toast.warning { border-left-color: var(--warn); }
    .toast.leaving { opacity: 0; transform: translateX(12px); }
    @keyframes toastIn { from { opacity: 0; transform: translateX(12px); } to { opacity: 1; transform: translateX(0); } }

    /* 少动效 + 可关（模板 Motion 节的精神：动只说明有事发生）。 */
    @media (prefers-reduced-motion: reduce) {
      .toast { animation: none; }
      .light, .nav, .button, .check-row input::after { transition: none; }
    }
  </style>
</head>)raw";

    // Part 2: Body markup
    html += R"raw(
<body>
  <div class="window-shell">
    <aside class="sidebar">
      <div class="sidebar-chrome">
        <div class="traffic">
          <button class="light close" id="btnWinClose" title="关闭"></button>
          <button class="light min" id="btnWinMin" title="最小化"></button>
          <button class="light max" id="btnWinMax" title="最大化"></button>
        </div>
      </div>
      <div class="sidebar-identity">
        <div class="title">
          <span class="title-mark">
            <svg viewBox="0 0 24 24"><path d="M12 3a9 9 0 1 0 9 9c0-.46-.04-.92-.1-1.36a5.389 5.389 0 0 1-4.4 2.26 5.403 5.403 0 0 1-3.14-9.8c-.44-.06-.9-.1-1.36-.1z"/></svg>
          </span>
          GeoDark
        </div>
        <div class="env-pill" id="envPill">
          <i></i>
          <span id="envPillText">后台：检测中</span>
        </div>
      </div>
      <div class="sidebar-navigation">
        <div class="sidebar-label">工作台</div>
        <button class="nav active" id="navStatus" data-view="status">
          <svg viewBox="0 0 24 24"><circle cx="12" cy="12" r="4"/><path d="M12 2v2M12 20v2M4.93 4.93l1.41 1.41M17.66 17.66l1.41 1.41M2 12h2M20 12h2M6.34 17.66l-1.41 1.41M19.07 4.93l-1.41 1.41"/></svg>
          <span>运行状态</span>
        </button>
        <div class="sidebar-label settings-label">偏好设置</div>
        <button class="nav" id="navSettings" data-view="settings">
          <svg viewBox="0 0 24 24"><circle cx="12" cy="12" r="3"/><path d="M19.4 15a1.65 1.65 0 0 0 .33 1.82l.06.06a2 2 0 0 1 0 2.83 2 2 0 0 1-2.83 0l-.06-.06a1.65 1.65 0 0 0-1.82-.33 1.65 1.65 0 0 0-1 1.51V21a2 2 0 0 1-2 2 2 2 0 0 1-2-2v-.09A1.65 1.65 0 0 0 9 19.4a1.65 1.65 0 0 0-1.82.33l-.06.06a2 2 0 0 1-2.83 0 2 2 0 0 1 0-2.83l.06-.06a1.65 1.65 0 0 0 .33-1.82 1.65 1.65 0 0 0-1.51-1H3a2 2 0 0 1-2-2 2 2 0 0 1 2-2h.09A1.65 1.65 0 0 0 4.6 9a1.65 1.65 0 0 0-.33-1.82l-.06-.06a2 2 0 0 1 0-2.83 2 2 0 0 1 2.83 0l.06.06a1.65 1.65 0 0 0 1.82.33H9a1.65 1.65 0 0 0 1-1.51V3a2 2 0 0 1 2-2 2 2 0 0 1 2 2v.09a1.65 1.65 0 0 0 1 1.51 1.65 1.65 0 0 0 1.82-.33l.06-.06a2 2 0 0 1 2.83 0 2 2 0 0 1 0 2.83l-.06.06a1.65 1.65 0 0 0-.33 1.82V9a1.65 1.65 0 0 0 1.51 1H21a2 2 0 0 1 2 2 2 2 0 0 1-2 2h-.09a1.65 1.65 0 0 0-1.51 1z"/></svg>
          <span>参数设置</span>
        </button>
      </div>

      <div class="environment-card">
        <div class="env-title">
          <span>环境与服务</span>
          <button id="btnRefreshEnv" title="刷新状态">
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
    </aside>

    <main class="main-area">
      <section class="content">
        <!-- VIEW: STATUS -->
        <div class="view" id="viewStatus">
          <div class="page-head">
            <div>
              <h1>运行状态</h1>
              <p>实时监控后台守护状态、定位坐标与下一次主题切换事件</p>
            </div>
            <div class="head-actions">
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
          </div>

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

          <div class="setting-section" style="padding: 16px 20px;">
            <div style="display:flex; justify-content:space-between; align-items:center; margin-bottom: 12px;">
              <h3 style="margin:0; font-size:12px; color:var(--muted); text-transform:uppercase; letter-spacing:0.04em;">实时运行控制台</h3>
              <span id="logTimestamp" style="font-size:10px; color:var(--muted);">轮询周期: 3 秒</span>
            </div>
            <div class="log-panel" id="logPanel"></div>
          </div>
        </div>

        <!-- VIEW: SETTINGS -->
        <div class="view hidden" id="viewSettings">
          <div class="page-head">
            <div>
              <h1>参数设置</h1>
              <p>配置太阳时间计算规则、定位模式与自动切换行为</p>
            </div>
            <div class="head-actions">
              <button class="button primary" id="btnSaveSettings">
                <svg viewBox="0 0 24 24"><path d="M19 21H5a2 2 0 0 1-2-2V5a2 2 0 0 1 2-2h11l5 5v11a2 2 0 0 1-2 2z"/><polyline points="17 21 17 13 7 13 7 21"/><polyline points="7 3 7 8 15 8"/></svg>
                保存设置
              </button>
            </div>
          </div>

          <form id="settingsForm" onsubmit="return false;">
            <section class="setting-section">
              <h3>运行模式</h3>
              <label class="check-row">
                <input type="checkbox" id="cfgAutoEnabled" checked>
                <span>
                  <strong>自动模式</strong>
                  <small>根据日出日落时间自动切换 Windows 应用与系统深浅色主题</small>
                </span>
              </label>
              <label class="check-row">
                <input type="checkbox" id="cfgStartupEnabled">
                <span>
                  <strong>登录 Windows 后自动运行</strong>
                  <small>系统开机登录后在后台静默运行，无需管理员权限</small>
                </span>
              </label>
            </section>

            <section class="setting-section">
              <h3>定位方式与坐标</h3>
              <label>
                定位方式
                <span>自动定位无需手动填写经纬度</span>
                <select id="cfgLocationMode">
                  <option value="0">Windows 自动定位</option>
                  <option value="1">手动坐标</option>
                </select>
              </label>

              <div class="two-col" style="margin-top: 14px;">
                <label>
                  纬度 (-90 ~ 90)
                  <span>例如 31.230416</span>
                  <input type="text" id="cfgLatitude" placeholder="0.000000" autocomplete="off" disabled>
                </label>
                <label>
                  经度 (-180 ~ 180)
                  <span>例如 121.473701</span>
                  <input type="text" id="cfgLongitude" placeholder="0.000000" autocomplete="off" disabled>
                </label>
              </div>
            </section>

            <section class="setting-section">
              <h3>日照偏移</h3>
              <div class="two-col">
                <label>
                  日出偏移（分钟）
                  <span>正数推迟，负数提前</span>
                  <input type="number" id="cfgSunriseOffset" min="-120" max="120" value="0">
                </label>
                <label>
                  日落偏移（分钟）
                  <span>范围 -120 到 +120</span>
                  <input type="number" id="cfgSunsetOffset" min="-120" max="120" value="0">
                </label>
              </div>
              <div class="form-hint">
                说明：偏移量可微调切换时机。例如设置日落偏移 +30 分钟，系统将在日落 30 分钟后才切换至深色模式。
              </div>
            </section>
          </form>
        </div>

      </section>
    </main>
  </div>

  <div class="toast-stack" id="toastStack"></div>)raw";

    // Part 3: JavaScript
    html += R"raw(
  <script>
    let isEditingCoordinates = false;

    // 主题应用：显式 .dark class（C++ 推送的 theme.apps 为唯一权威；
    // prefers-color-scheme 仅在初始 head 脚本里作回退起点）。
    function applyThemeClass(appsValue) {
      const dark = (appsValue === 0);
      document.documentElement.classList.toggle("dark", dark);
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

    const navStatus = document.getElementById("navStatus");
    const navSettings = document.getElementById("navSettings");
    const viewStatus = document.getElementById("viewStatus");
    const viewSettings = document.getElementById("viewSettings");

    function switchView(view) {
      if (view === "settings") {
        navSettings.classList.add("active");
        navStatus.classList.remove("active");
        viewSettings.classList.remove("hidden");
        viewStatus.classList.add("hidden");
      } else {
        navStatus.classList.add("active");
        navSettings.classList.remove("active");
        viewStatus.classList.remove("hidden");
        viewSettings.classList.add("hidden");
      }
    }

    navStatus.addEventListener("click", () => switchView("status"));
    navSettings.addEventListener("click", () => switchView("settings"));

    document.getElementById("btnWinClose").addEventListener("click", () => {
      window.chrome.webview.postMessage({ action: "window_close" });
    });
    document.getElementById("btnWinMin").addEventListener("click", () => {
      window.chrome.webview.postMessage({ action: "window_min" });
    });
    document.getElementById("btnWinMax").addEventListener("click", () => {
      window.chrome.webview.postMessage({ action: "window_max" });
    });

    const chromeBar = document.querySelector(".sidebar-chrome");
    chromeBar.addEventListener("mousedown", (e) => {
      if (e.target.closest(".traffic")) return;
      window.chrome.webview.postMessage({ action: "window_drag" });
    });

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

    document.getElementById("btnSaveSettings").addEventListener("click", () => {
      const data = {
        auto_enabled: autoCheck.checked,
        location_mode: parseInt(modeSelect.value, 10),
        latitude: latInput.value.trim(),
        longitude: lonInput.value.trim(),
        sunrise_offset: document.getElementById("cfgSunriseOffset").value.trim(),
        sunset_offset: document.getElementById("cfgSunsetOffset").value.trim(),
        startup_enabled: document.getElementById("cfgStartupEnabled").checked
      };
      window.chrome.webview.postMessage({ action: "save", data });
    });

    document.getElementById("btnManualLight").addEventListener("click", () => {
      window.chrome.webview.postMessage({ action: "switch_theme", light: true });
    });

    document.getElementById("btnManualDark").addEventListener("click", () => {
      window.chrome.webview.postMessage({ action: "switch_theme", light: false });
    });

    document.getElementById("btnRefreshStatus").addEventListener("click", () => {
      window.chrome.webview.postMessage({ action: "refresh_status" });
    });

    document.getElementById("btnRefreshEnv").addEventListener("click", () => {
      window.chrome.webview.postMessage({ action: "refresh_status" });
    });

    document.getElementById("btnQuickRefreshLoc").addEventListener("click", () => {
      window.chrome.webview.postMessage({ action: "refresh_location" });
    });

    document.getElementById("btnQuickAuth").addEventListener("click", () => {
      window.chrome.webview.postMessage({ action: "authorize" });
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

    window.chrome.webview.postMessage({ action: "init" });
  </script>
</body>
</html>)raw";

    return html;
}

} // namespace geodark
