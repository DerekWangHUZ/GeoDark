#pragma once

#include <string>

namespace geodark {

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
  <style>
    :root {
      font-family: Inter, "SF Pro Display", "Segoe UI", system-ui, -apple-system, sans-serif;
      font-optical-sizing: auto;
      color: #1d1d1f;
      background: transparent;
      font-synthesis: none;
      text-size-adjust: 100%;
      -webkit-text-size-adjust: 100%;
      -webkit-font-smoothing: antialiased;
      -moz-osx-font-smoothing: grayscale;
      text-rendering: optimizeLegibility;
      --blue: #007aff;
      --blue-dark: #0066d6;
      --surface: rgba(255, 255, 255, 0.86);
      --border: rgba(0, 0, 0, 0.09);
      --muted: #6e6e73;
      --sidebar: rgba(239, 239, 242, 0.88);
    }
    * { box-sizing: border-box; user-select: none; }
    ::-webkit-scrollbar { width: 7px; height: 7px; }
    ::-webkit-scrollbar-track { background: transparent; }
    ::-webkit-scrollbar-thumb { background: rgba(128, 128, 136, 0.28); border-radius: 4px; }
    ::-webkit-scrollbar-thumb:hover { background: rgba(128, 128, 136, 0.48); }
    html, body, #app {
      margin: 0; width: 100%; height: 100%; overflow: hidden;
    }
    button, input, select { font: inherit; }
    button { color: inherit; }
    svg {
      width: 18px; height: 18px; fill: none; stroke: currentColor;
      stroke-width: 1.8; stroke-linecap: round; stroke-linejoin: round;
      shape-rendering: geometricPrecision;
    }
    .window-shell {
      height: 100%; display: flex; flex-direction: row; background: #f5f5f7; overflow: hidden;
    }
    .main-area {
      position: relative; min-width: 0; min-height: 0; flex: 1; display: flex; flex-direction: column; overflow: hidden;
    }
    .sidebar {
      width: 224px; height: 100%; flex: 0 0 224px; padding: 0 13px 16px;
      background: var(--sidebar); border-right: 1px solid var(--border); display: flex; flex-direction: column;
    }
    .sidebar-chrome {
      height: 52px; flex: 0 0 52px; margin: 0 -13px; padding: 0 16px; display: flex; align-items: center; cursor: default;
    }
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
    .title-mark {
      display: flex; align-items: center; justify-content: center; width: 24px; height: 24px; border-radius: 6px;
      background: linear-gradient(135deg, #ff9500 0%, #007aff 100%); color: white; box-shadow: 0 2px 6px rgba(0, 122, 255, 0.25);
    }
    .title-mark svg { width: 15px; height: 15px; stroke-width: 2.2; }
    .env-pill {
      width: 100%; min-width: 0; display: flex; align-items: center; gap: 7px; padding: 5px 8px; border-radius: 7px;
      background: rgba(0, 0, 0, 0.045); color: var(--muted); font-size: 11px; font-weight: 600;
    }
    .env-pill span { min-width: 0; overflow: hidden; text-overflow: ellipsis; white-space: nowrap; }
    .env-pill i {
      width: 7px; height: 7px; border-radius: 50%; background: #ff453a;
      box-shadow: 0 0 0 3px rgba(255, 69, 58, 0.15); flex: 0 0 auto;
    }
    .env-pill.ok i { background: #30b859; box-shadow: 0 0 0 3px rgba(48, 184, 89, 0.15); }
    .sidebar-label {
      margin: 0 10px 8px; color: #8a8a8e; font-size: 11px; font-weight: 650; text-transform: uppercase; letter-spacing: 0.04em;
    }
    .settings-label { margin-top: 25px; }
    .nav {
      width: 100%; border: 0; background: transparent; border-radius: 7px; display: flex; align-items: center; gap: 10px;
      padding: 8px 10px; margin: 2px 0; font-size: 13px; font-weight: 520; cursor: pointer; text-align: left;
      transition: background 0.15s, transform 0.1s;
    }
    .nav:hover { background: rgba(0, 0, 0, 0.045); }
    .nav:active { transform: scale(0.985); }
    .nav.active { background: rgba(0, 122, 255, 0.13); color: #006ee5; font-weight: 600; }
    .nav svg { width: 17px; height: 17px; }
    .nav span { flex: 1; }
    .environment-card {
      margin-top: auto; padding: 12px; border: 1px solid var(--border); border-radius: 10px;
      background: rgba(255, 255, 255, 0.5); font-size: 11px;
    }
    .environment-card > div:not(.card-actions) {
      display: flex; align-items: center; gap: 7px; margin-top: 8px; color: var(--muted);
    }
    .environment-card .env-title {
      margin: 0 0 4px !important; color: #3a3a3c !important; font-weight: 650; display: flex; align-items: center;
    }
    .env-title span { flex: 1; }
    .env-title button {
      border: 0; background: transparent; cursor: pointer; color: var(--muted); padding: 2px; border-radius: 4px; display: flex; align-items: center;
    }
    .env-title button:hover { background: rgba(0, 0, 0, 0.06); color: var(--blue); }
    .environment-card i { width: 7px; height: 7px; border-radius: 50%; background: #ff453a; flex: 0 0 auto; }
    .environment-card i.ok { background: #30b859; }
    .environment-card i.warn { background: #f0a000; }
    .environment-card b { margin-left: auto; font-size: 10px; font-weight: 550; color: #1d1d1f; }
    .card-actions { display: grid; grid-template-columns: 1fr 1fr; gap: 6px; margin-top: 10px; }
    .card-action-btn {
      width: 100%; border: 1px solid var(--border); border-radius: 6px; padding: 6px; color: #3a3a3c;
      background: white; font-size: 10px; font-weight: 600; cursor: pointer; display: inline-flex; align-items: center; justify-content: center; gap: 4px; transition: background 0.15s;
    }
    .card-action-btn:hover:not(:disabled) { background: #f0f0f4; }
    .card-action-btn:disabled { opacity: 0.45; cursor: default; }
    .content { min-width: 0; min-height: 0; flex: 1; overflow: hidden; background: #f7f7f9; }
    .view { height: 100%; overflow-y: auto; padding: 30px clamp(24px, 4vw, 50px) 40px; }
    .view.hidden { display: none !important; }
    .page-head { display: flex; align-items: flex-start; justify-content: space-between; gap: 20px; margin-bottom: 24px; }
    h1 { margin: 0 0 5px; font-size: 26px; letter-spacing: -0.035em; font-weight: 720; }
    .page-head p { margin: 0; color: var(--muted); font-size: 13px; }
    .head-actions { display: flex; gap: 8px; }
    .button {
      height: 34px; padding: 0 14px; border-radius: 7px; border: 1px solid var(--border); background: white;
      display: inline-flex; align-items: center; justify-content: center; gap: 6px; font-size: 12px; font-weight: 620; cursor: pointer;
      box-shadow: 0 1px 2px rgba(0, 0, 0, 0.04); transition: transform 0.12s ease, box-shadow 0.12s ease, background 0.15s ease;
    }
    .button:hover:not(:disabled) { transform: translateY(-1px); box-shadow: 0 3px 8px rgba(0, 0, 0, 0.09); }
    .button:active:not(:disabled) { transform: scale(0.98); }
    .button.primary {
      color: white; border-color: transparent; background: linear-gradient(#1687ff, #0072ed); box-shadow: 0 1px 2px rgba(0, 85, 190, 0.3);
    }
    .button.primary:hover:not(:disabled) { background: linear-gradient(#2490ff, #007aff); }
    .button:disabled { opacity: 0.48; cursor: default; }
    .button svg { width: 14px; height: 14px; }
    .setting-section {
      padding: 20px 22px; margin-bottom: 14px; background: white; border: 1px solid var(--border); border-radius: 12px;
    }
    .setting-section h3 { margin: 0 0 16px; font-size: 13px; font-weight: 650; letter-spacing: -0.01em; }
    label { display: block; color: #3a3a3c; font-size: 12px; font-weight: 600; }
    label > span { float: right; color: #8e8e93; font-size: 10px; font-weight: 400; }
    input, select {
      width: 100%; height: 36px; margin-top: 7px; padding: 0 11px; outline: 0;
      border: 1px solid rgba(0, 0, 0, 0.13); border-radius: 7px; background: rgba(248, 248, 250, 0.85);
      color: inherit; font-size: 12px; transition: border 0.15s, box-shadow 0.15s, background 0.15s; user-select: text;
    }
    input:focus, select:focus { border-color: var(--blue); box-shadow: 0 0 0 3px rgba(0, 122, 255, 0.11); background: white; }
    input:disabled, select:disabled { opacity: 0.45; background: rgba(0, 0, 0, 0.04); cursor: not-allowed; }
    .two-col { display: grid; grid-template-columns: 1fr 1fr; gap: 14px; }
    .setting-section label + label, .setting-section .two-col + label, .setting-section label + .two-col, .setting-section .check-row + .check-row { margin-top: 14px; }
    .check-row {
      display: flex; align-items: center; gap: 12px; padding: 12px; border: 1px solid var(--border);
      border-radius: 8px; background: rgba(248, 248, 250, 0.7); cursor: pointer;
    }
    .check-row input {
      appearance: none; -webkit-appearance: none; flex: 0 0 auto; width: 36px; height: 20px;
      margin: 0; padding: 0; border: 0; border-radius: 10px; background: #c7c7cc; position: relative; cursor: pointer;
      box-shadow: none; transition: background 0.2s;
    }
    .check-row input::after {
      content: ""; position: absolute; top: 2px; left: 2px; width: 16px; height: 16px;
      border-radius: 50%; background: white; box-shadow: 0 1px 3px rgba(0, 0, 0, 0.25);
      transition: transform 0.2s cubic-bezier(0.2, 0.8, 0.2, 1);
    }
    .check-row input:checked { background: #30b859; }
    .check-row input:checked::after { transform: translateX(16px); }
    .check-row span { float: none; flex: 1; }
    .check-row strong { display: block; font-size: 12px; font-weight: 600; }
    .check-row small { display: block; margin-top: 2px; color: var(--muted); font-size: 10px; font-weight: 400; }
    .form-hint { margin-top: 10px; color: var(--muted); font-size: 11px; }
    .status-grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(240px, 1fr)); gap: 14px; margin-bottom: 20px; }
    .status-card {
      background: white; border: 1px solid var(--border); border-radius: 12px; padding: 16px 18px; box-shadow: 0 2px 8px rgba(0, 0, 0, 0.03);
    }
    .status-card-top { display: flex; align-items: center; gap: 10px; margin-bottom: 12px; }
    .status-card-icon {
      width: 32px; height: 32px; border-radius: 8px; display: grid; place-items: center; background: rgba(0, 122, 255, 0.1); color: var(--blue);
    }
    .status-card-icon.sun { background: rgba(255, 149, 0, 0.12); color: #ff9500; }
    .status-card-icon.moon { background: rgba(88, 86, 214, 0.12); color: #5856d6; }
    .status-card-title { flex: 1; font-size: 13px; font-weight: 650; }
    .status-tag {
      padding: 3px 8px; border-radius: 99px; font-size: 10px; font-weight: 650;
      background: rgba(48, 184, 89, 0.1); color: #248b43; display: inline-flex; align-items: center; gap: 4px;
    }
    .status-tag i { width: 6px; height: 6px; border-radius: 50%; background: currentColor; }
    .status-tag.warn { background: rgba(240, 160, 0, 0.12); color: #946700; }
    .status-tag.bad { background: rgba(255, 69, 58, 0.12); color: #c72e27; }
    .status-meta { display: grid; gap: 6px; font-size: 11px; }
    .status-meta-row { display: flex; justify-content: space-between; color: var(--muted); }
    .status-meta-row b { color: #1d1d1f; font-weight: 550; }
    .log-panel {
      background: #1d1d20; border-radius: 12px; padding: 14px 0; overflow-y: auto; max-height: 320px; min-height: 200px;
      color: #d7d7dc; box-shadow: inset 0 1px 3px rgba(0, 0, 0, 0.35);
    }
    .log-line {
      display: flex; gap: 12px; padding: 4px 16px; font: 11px/1.5 "Cascadia Mono", Consolas, monospace; user-select: text;
    }
    .log-line time { flex: 0 0 65px; color: #67676e; }
    .log-line span { white-space: pre-wrap; overflow-wrap: anywhere; }
    .log-line.cmd span { color: #68a8ff; }
    .log-line.ok span { color: #5cdb7e; }
    .log-line.warn span { color: #ffd06a; }
    .log-line.err span { color: #ff837c; }
    .toast-stack { position: fixed; z-index: 200; top: 18px; right: 18px; display: flex; flex-direction: column; gap: 8px; pointer-events: none; }
    .toast {
      pointer-events: auto; min-width: 260px; max-width: 380px; padding: 11px 14px; border: 1px solid var(--border);
      border-left: 3px solid var(--blue); border-radius: 9px; background: rgba(255, 255, 255, 0.96); backdrop-filter: blur(12px);
      box-shadow: 0 8px 30px rgba(0, 0, 0, 0.15); font-size: 12px; font-weight: 500; animation: toastIn 0.2s ease-out; transition: opacity 0.2s, transform 0.2s;
    }
    .toast.success { border-left-color: #30b859; }
    .toast.error { border-left-color: #ff453a; }
    .toast.warning { border-left-color: #f0a000; }
    .toast.leaving { opacity: 0; transform: translateX(12px); }
    @keyframes toastIn { from { opacity: 0; transform: translateX(12px); } to { opacity: 1; transform: translateX(0); } }
    @media (prefers-color-scheme: dark) {
      :root {
        color: #f2f2f7; --surface: rgba(43, 43, 46, 0.9); --border: rgba(255, 255, 255, 0.09);
        --muted: #98989f; --sidebar: rgba(37, 37, 40, 0.94);
      }
      .window-shell, .content { background: #1e1e20; }
      .nav:hover { background: rgba(255, 255, 255, 0.05); }
      .nav.active { background: rgba(0, 122, 255, 0.22); color: #5aa7ff; }
      .environment-card { background: rgba(255, 255, 255, 0.035); }
      .environment-card .env-title { color: #e4e4e8 !important; }
      .environment-card b { color: #eeeef2; }
      .card-action-btn { background: #2f2f32; color: #dddde2; }
      .card-action-btn:hover:not(:disabled) { background: #3a3a3e; }
      .setting-section, .status-card { background: rgba(47, 47, 50, 0.88); }
      label { color: #d8d8dc; }
      .status-meta-row b { color: #eeeef2; }
      .button { background: #3b3b3e; color: #eeeef2; }
      .button:hover:not(:disabled) { background: #46464a; }
      input, select, .check-row { color: #eeeef2; background: #29292c; border-color: rgba(255, 255, 255, 0.12); }
      input:focus, select:focus { background: #2f2f33; }
      .check-row input { background: #5a5a60; }
      .check-row input:checked { background: #30b859; }
      .toast { background: rgba(48, 48, 51, 0.98); color: #f2f2f7; }
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

      if (msg.state) {
        renderState(msg.state, msg.config, msg.theme, msg.startup);
      }
    });

    function formatTime(str) {
      return str || "未取得";
    }

    function renderState(state, config, theme, startup) {
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
