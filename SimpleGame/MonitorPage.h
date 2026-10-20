#pragma once

// The monitor's page, as the game hands it to the browser: it listens on
// /events and redraws itself from each snapshot. Kept in the program itself,
// so there is no file to lose. (Split in two: one literal can only be so long.)
const char* const kMonitorPage = R"PAGE(<!doctype html>
<html lang="ko">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Darkest Light Monitor</title>
<style>
:root { --bg:#141317; --panel:#1d1c22; --line:#34323b; --text:#e9e5dc; --dim:#9a968e; --gold:#f2c35a; --violet:#8f66e0; --ash:#9e9ea3; --bad:#d9574a; }
* { box-sizing:border-box; }
body { margin:0; background:var(--bg); color:var(--text); font:14px/1.45 "Malgun Gothic", system-ui, sans-serif; }
header { display:flex; flex-wrap:wrap; gap:12px 24px; align-items:baseline; padding:14px 16px; border-bottom:1px solid var(--line); }
h1 { font-size:18px; margin:0; }
h2 { font-size:15px; margin:0 0 8px; }
#status { color:var(--dim); }
#status.live { color:var(--gold); }
#status.lost { color:var(--bad); }
main { padding:16px; display:grid; gap:16px; grid-template-columns:repeat(auto-fit, minmax(320px, 1fr)); }
section { background:var(--panel); border:1px solid var(--line); border-radius:6px; padding:12px; overflow-x:auto; }
section.wide { grid-column:1 / -1; }
table { border-collapse:collapse; width:100%; }
th, td { text-align:left; padding:4px 8px; border-bottom:1px solid var(--line); white-space:nowrap; }
th { color:var(--dim); font-weight:normal; }
td.line { white-space:normal; min-width:240px; color:var(--dim); }
.Light { color:var(--gold); } .Dark { color:var(--violet); } .Ashen { color:var(--ash); }
.yes { color:var(--gold); } .no { color:var(--dim); }
.count { font-size:22px; }
.counts { display:flex; flex-wrap:wrap; gap:8px 20px; }
.counts div span { display:block; color:var(--dim); font-size:12px; }
</style>
</head>
<body>
<header>
<h1>The Darkest Light · 모니터</h1>
<span id="status">연결 중…</span>
<span id="where"></span>
<span id="time"></span>
</header>
<main>
<section><h2>수</h2><div class="counts" id="counts"></div></section>
<section class="wide"><h2>마을</h2><table><thead><tr><th>이름</th><th>양식</th><th>신전</th><th>NPC</th><th>몬스터</th><th>가 봄</th><th>의뢰</th></tr></thead><tbody id="villages"></tbody></table></section>
<section class="wide"><h2>NPC</h2><table><thead><tr><th>#</th><th>종류</th><th>양식</th><th>지역</th><th>위치</th><th>대화 중</th><th>신의 영향</th><th>오류 반응</th><th>지금 대사</th></tr></thead><tbody id="npcs"></tbody></table></section>
</main>
)PAGE"
R"PAGE(<script>
const styleNames = { Light:"빛", Dark:"어둠", Ashen:"잿빛", Borderborn:"경계", Mixed:"섞임" };
function esc(text) {
  return String(text).replace(/[&<>"]/g, c => ({ "&":"&amp;", "<":"&lt;", ">":"&gt;", "\"":"&quot;" })[c]);
}
function mark(flag) { return flag ? '<span class="yes">예</span>' : '<span class="no">아니오</span>'; }
function style(name) { return '<span class="' + esc(name) + '">' + esc(styleNames[name] || name) + '</span>'; }

function draw(s) {
  document.getElementById("where").textContent = "지금 있는 곳 · " + s.region.name;
  document.getElementById("time").textContent = "게임 시간 " + s.time.toFixed(1) + "초";
  const c = s.counts, k = c.monstersByKind;
  const counts = [["몬스터", c.monsters], ["빛 기사", k.lightKnight], ["빛 마법사", k.lightMage],
    ["어둠 기사", k.darkKnight], ["어둠 마법사", k.darkMage], ["NPC", c.npcs]];
  document.getElementById("counts").innerHTML = counts.map(p =>
    '<div><span>' + p[0] + '</span><b class="count">' + p[1] + '</b></div>').join("");
  document.getElementById("villages").innerHTML = s.villages.map(v =>
    '<tr><td>' + esc(v.name) + '</td><td>' + style(v.style) + '</td><td>' + mark(v.temple) + '</td><td>' + v.npcs +
    '</td><td>' + (v.monsters === null ? '<span class="no">놓이지 않음</span>' : v.monsters) + '</td><td>' + mark(v.visited) +
    '</td><td>' + v.requests + '</td></tr>').join("");
  document.getElementById("npcs").innerHTML = s.npcs.map(n =>
    '<tr><td>' + n.id + '</td><td>' + esc(n.kind) + '</td><td>' + style(n.style) + '</td><td>' + esc(n.region) +
    '</td><td>' + n.col.toFixed(1) + ', ' + n.row.toFixed(1) + '</td><td>' + mark(n.talking) + '</td><td>' + mark(n.influenced) +
    '</td><td>' + mark(n.errorReaction) + '</td><td class="line">' + esc(n.line) + '</td></tr>').join("");
}

function connect() {
  const status = document.getElementById("status");
  const events = new EventSource("/events");
  events.onopen = () => { status.textContent = "연결됨"; status.className = "live"; };
  events.onmessage = e => { try { draw(JSON.parse(e.data)); } catch (err) { console.error(err); } };
  events.onerror = () => { status.textContent = "연결 끊김 · 게임을 기다리는 중"; status.className = "lost"; };
}
connect();
</script>
</body>
</html>
)PAGE";
