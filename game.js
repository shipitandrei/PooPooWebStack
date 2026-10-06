(function () {
  "use strict";
  var W = 1920, H = 1080;
  var canvas = document.getElementById("game");
  var stage = document.getElementById("stage");
  var ctx = canvas.getContext("2d");
  var keys = {};
  var lastTime = 0;
  var bestScore = 0;
  var debugMode = false;
  var debugTapCount = 0;
  var debugTapStartedAt = 0;
  var toiletSprite = loadSprite("assets/toilet.png");
  var pooSprite = loadSprite("assets/poo.png");
  try { bestScore = Number(window.localStorage.getItem("poopoo-best") || 0) || 0; } catch (ignore) {}

  var playButton = { x: 760, y: 590, w: 400, h: 130 };
  var backgroundMusic = document.createElement("audio");
  backgroundMusic.src = "assets/music.mp3";
  backgroundMusic.loop = true;
  backgroundMusic.preload = "none";
  backgroundMusic.volume = 0.4;
  var musicUnavailable = false;
  backgroundMusic.addEventListener("error", function () { musicUnavailable = true; });

  function startMusic() {
    if (musicUnavailable) return;
    try {
      var request = backgroundMusic.play();
      if (request && request.catch) request.catch(function () {});
    } catch (ignore) {}
  }

  var game = {
    mode: fitsTargetViewport() ? "menu" : "gate",
    resumeModeAfterGate: "menu",
    playerX: W / 2,
    targetX: W / 2,
    score: 0,
    lives: 3,
    poos: [],
    spawnIn: 0.4,
    flash: 0
  };

  function loadSprite(path) {
    var image = new Image();
    var asset = { image: image, loaded: false, path: path };
    image.onload = function () { asset.loaded = true; };
    image.onerror = function () { asset.loaded = false; };
    image.src = path;
    return asset;
  }

  function fitsTargetViewport() {
    return debugMode || (window.innerWidth >= W && window.innerHeight >= H);
  }

  function resizeStage() {
    var scale = Math.min(window.innerWidth / W, window.innerHeight / H);
    if (!isFinite(scale) || scale <= 0) scale = 0.5;
    stage.style.transform = "translate(-50%, -50%) scale(" + scale + ")";

    if (!fitsTargetViewport()) {
      if (game.mode !== "gate") {
        game.resumeModeAfterGate = game.mode;
        game.mode = "gate";
      }
      return;
    }

    if (game.mode === "gate") {
      game.mode = game.resumeModeAfterGate || "menu";
      game.resumeModeAfterGate = "menu";
    }
  }

  function requestFullscreen() {
    var root = document.documentElement;
    try {
      var result;
      if (root.requestFullscreen) result = root.requestFullscreen();
      else if (root.webkitRequestFullscreen) result = root.webkitRequestFullscreen();
      else if (root.webkitRequestFullScreen) result = root.webkitRequestFullScreen();
      if (result && result.catch) result.catch(function () {});
    } catch (ignore) {}
  }

  function roundedRect(x, y, w, h, r, fill, stroke, lineWidth) {
    r = Math.min(r, w / 2, h / 2);
    ctx.beginPath();
    ctx.moveTo(x + r, y);
    ctx.lineTo(x + w - r, y);
    ctx.quadraticCurveTo(x + w, y, x + w, y + r);
    ctx.lineTo(x + w, y + h - r);
    ctx.quadraticCurveTo(x + w, y + h, x + w - r, y + h);
    ctx.lineTo(x + r, y + h);
    ctx.quadraticCurveTo(x, y + h, x, y + h - r);
    ctx.lineTo(x, y + r);
    ctx.quadraticCurveTo(x, y, x + r, y);
    ctx.closePath();
    if (fill) { ctx.fillStyle = fill; ctx.fill(); }
    if (stroke) { ctx.strokeStyle = stroke; ctx.lineWidth = lineWidth || 2; ctx.stroke(); }
  }

  function drawAssetPlaceholder(asset, x, y, w, h, label) {
    roundedRect(x, y, w, h, 12, "rgba(12,20,25,0.60)", "rgba(237,216,164,0.68)", 3);
    ctx.textAlign = "center";
    ctx.textBaseline = "middle";
    ctx.fillStyle = "#e5bd68";
    ctx.font = "700 16px Arial, Helvetica, sans-serif";
    ctx.fillText(asset.path, x + w / 2, y + h / 2 - 11);
    ctx.fillStyle = "rgba(245,240,228,0.70)";
    ctx.font = "500 14px Arial, Helvetica, sans-serif";
    ctx.fillText(label + " PNG NEEDED", x + w / 2, y + h / 2 + 17);
  }

  function drawSprite(asset, x, y, w, h, label) {
    if (asset.loaded) ctx.drawImage(asset.image, x, y, w, h);
    else drawAssetPlaceholder(asset, x, y, w, h, label);
  }

  function drawPoo(x, y, size) {
    drawSprite(pooSprite, x - size / 2, y - size / 2, size, size, "POO");
  }

  function drawToilet(x) {
    drawSprite(toiletSprite, x - 140, H - 240, 280, 230, "TOILET");
  }

  function drawGate() {
    ctx.fillStyle = "#101820";
    ctx.fillRect(0, 0, W, H);
    ctx.fillStyle = "rgba(226,190,108,0.10)";
    ctx.beginPath(); ctx.arc(W / 2, H / 2, 158, 0, Math.PI * 2); ctx.fill();
    ctx.font = "600 48px Arial, Helvetica, sans-serif";
    ctx.textBaseline = "middle";
    ctx.textAlign = "left";
    var label = "Press";
    var labelWidth = ctx.measureText(label).width;
    var boxSize = 52;
    var gap = 22;
    var total = labelWidth + gap + boxSize;
    var left = (W - total) / 2;
    ctx.fillStyle = "#f3eddf";
    ctx.fillText(label, left, H / 2);
    ctx.strokeStyle = "#e4bd68";
    ctx.lineWidth = 5;
    ctx.strokeRect(left + labelWidth + gap, H / 2 - boxSize / 2, boxSize, boxSize);
    ctx.textAlign = "center";
    ctx.fillStyle = "rgba(243,237,223,0.62)";
    ctx.font = "500 18px Arial, Helvetica, sans-serif";
    ctx.fillText("Made by kuayne", W / 2, H / 2 + 70);
  }

  function drawMenu() {
    ctx.fillStyle = "rgba(8,17,24,0.72)";
    ctx.fillRect(0, 0, W, H);
    roundedRect(405, 275, 1110, 500, 32, "rgba(16,24,32,0.94)", "rgba(228,189,104,0.72)", 3);
    ctx.textAlign = "center";
    ctx.textBaseline = "middle";
    ctx.fillStyle = "#f3eddf";
    ctx.font = "800 76px Arial, Helvetica, sans-serif";
    ctx.fillText("Poo Poo on the Toilet", W / 2, 425);
    roundedRect(playButton.x, playButton.y, playButton.w, playButton.h, 22, "#e4bd68", "#fff0c7", 4);
    ctx.fillStyle = "#17242a";
    ctx.font = "800 46px Arial, Helvetica, sans-serif";
    ctx.fillText("PLAY", W / 2, playButton.y + playButton.h / 2);
  }

  function startGame() {
    game.mode = "play";
    game.playerX = W / 2;
    game.targetX = W / 2;
    game.score = 0;
    game.lives = 3;
    game.poos = [];
    game.spawnIn = 0.4;
    game.flash = 0;
    game.resumeModeAfterGate = "menu";
  }

  function saveBest() {
    if (game.score > bestScore) {
      bestScore = game.score;
      try { window.localStorage.setItem("poopoo-best", String(bestScore)); } catch (ignore) {}
    }
  }

  function moveToPointer(event) {
    if (game.mode !== "play") return;
    var rect = canvas.getBoundingClientRect();
    if (!rect.width) return;
    var x = (event.clientX - rect.left) * W / rect.width;
    var boostedX = W / 2 + (x - W / 2) * 1.15;
    game.targetX = Math.max(140, Math.min(W - 140, boostedX));
  }
  canvas.addEventListener("mousemove", moveToPointer);
  canvas.addEventListener("pointermove", moveToPointer);

  window.addEventListener("keydown", function (event) {
    var key = event.key || event.keyCode;
    if (key === "ArrowLeft" || key === "ArrowRight") {
      keys[key] = true;
      event.preventDefault();
    }
  });
  window.addEventListener("keyup", function (event) {
    var key = event.key || event.keyCode;
    if (key === "ArrowLeft" || key === "ArrowRight") keys[key] = false;
  });
  window.addEventListener("blur", function () { keys = {}; });

  function registerDebugTap() {
    if (debugMode) return false;
    var now = Date.now();
    if (!debugTapStartedAt || now - debugTapStartedAt > 2000) {
      debugTapCount = 0;
      debugTapStartedAt = now;
    }
    debugTapCount++;
    if (debugTapCount < 5) return false;
    debugTapCount = 0;
    debugTapStartedAt = 0;
    debugMode = true;
    if (game.mode === "gate") {
      game.resumeModeAfterGate = "menu";
      resizeStage();
    }
    return true;
  }

  canvas.addEventListener("click", function (event) {
    if (registerDebugTap()) return;
    if (game.mode === "gate") {
      startMusic();
      requestFullscreen();
    } else if (game.mode === "menu") {
      var rect = canvas.getBoundingClientRect();
      if (!rect.width || !rect.height) return;
      var x = (event.clientX - rect.left) * W / rect.width;
      var y = (event.clientY - rect.top) * H / rect.height;
      if (x >= playButton.x && x <= playButton.x + playButton.w && y >= playButton.y && y <= playButton.y + playButton.h) {
        startMusic();
        startGame();
      }
    } else if (game.mode === "over") startGame();
  });

  function update(dt) {
    if (game.mode !== "play") return;
    var direction = (keys.ArrowRight ? 1 : 0) - (keys.ArrowLeft ? 1 : 0);
    if (direction) {
      game.playerX += direction * 1800 * dt;
      game.targetX = game.playerX;
    } else {
      var follow = 1 - Math.exp(-20 * dt);
      game.playerX += (game.targetX - game.playerX) * follow;
    }
    game.playerX = Math.max(140, Math.min(W - 140, game.playerX));
    game.targetX = Math.max(140, Math.min(W - 140, game.targetX));
    game.flash = Math.max(0, game.flash - dt);
    game.spawnIn -= dt;
    if (game.spawnIn <= 0) {
      var speed = 260 + Math.min(250, game.score * 8);
      var size = 62 + Math.random() * 18;
      game.poos.push({ x: 100 + Math.random() * (W - 200), y: -55, size: size, speed: speed + Math.random() * 100 });
      game.spawnIn = Math.max(0.32, 0.88 - game.score * 0.014) + Math.random() * 0.28;
    }
    for (var i = game.poos.length - 1; i >= 0; i--) {
      var poo = game.poos[i];
      poo.y += poo.speed * dt;
      if (poo.y + poo.size / 2 >= H - 245 && poo.y < H - 95 && Math.abs(poo.x - game.playerX) < 145) {
        game.poos.splice(i, 1);
        game.score++;
        game.flash = 0.15;
        saveBest();
      } else if (poo.y - poo.size / 2 > H) {
        game.poos.splice(i, 1);
        game.lives--;
        if (game.lives <= 0) {
          saveBest();
          game.mode = "over";
        }
      }
    }
  }

  function drawHud() {
    roundedRect(54, 48, 344, 104, 18, "rgba(15,27,34,0.84)", "rgba(230,211,162,0.32)", 2);
    ctx.textAlign = "left";
    ctx.textBaseline = "middle";
    ctx.fillStyle = "#d7bd7f";
    ctx.font = "700 18px Arial, Helvetica, sans-serif";
    ctx.fillText("SCORE", 84, 80);
    ctx.fillStyle = "#f6f1e4";
    ctx.font = "700 42px Arial, Helvetica, sans-serif";
    ctx.fillText(String(game.score), 84, 122);
    roundedRect(W - 397, 48, 344, 104, 18, "rgba(15,27,34,0.84)", "rgba(230,211,162,0.32)", 2);
    ctx.fillStyle = "#d7bd7f";
    ctx.font = "700 18px Arial, Helvetica, sans-serif";
    ctx.fillText("MISSES", W - 365, 80);
    for (var i = 0; i < 3; i++) {
      ctx.fillStyle = i < game.lives ? "#e4bd68" : "rgba(228,189,104,0.20)";
      ctx.beginPath(); ctx.arc(W - 348 + i * 72, 121, 14, 0, Math.PI * 2); ctx.fill();
    }
    if (game.flash > 0) {
      ctx.fillStyle = "rgba(235,195,105," + (game.flash * 0.42) + ")";
      ctx.fillRect(0, 0, W, H);
      ctx.fillStyle = "#fff1c6";
      ctx.font = "800 38px Arial, Helvetica, sans-serif";
      ctx.textAlign = "center";
      ctx.fillText("CAUGHT! +1", W / 2, 255);
    }
  }

  function drawGameOver() {
    ctx.fillStyle = "rgba(9,16,21,0.78)";
    ctx.fillRect(0, 0, W, H);
    roundedRect(570, 335, 780, 390, 25, "#182a35", "#d7b567", 3);
    ctx.textAlign = "center";
    ctx.textBaseline = "middle";
    ctx.fillStyle = "#e5bd68";
    ctx.font = "700 22px Arial, Helvetica, sans-serif";
    ctx.fillText("POO POO ON THE TOILET", W / 2, 415);
    ctx.fillStyle = "#f6f1e4";
    ctx.font = "900 55px Arial, Helvetica, sans-serif";
    ctx.fillText("you got bing", W / 2, 492);
    ctx.fillStyle = "#c8d1c9";
    ctx.font = "500 25px Arial, Helvetica, sans-serif";
    ctx.fillText("Score: " + game.score + "     Best: " + bestScore, W / 2, 565);
    roundedRect(760, 615, 400, 70, 12, "#e3bc68", "#f8e2a9", 3);
    ctx.fillStyle = "#17242a";
    ctx.font = "700 23px Arial, Helvetica, sans-serif";
    ctx.fillText("x to play again", W / 2, 650);
  }

  function draw() {
    if (game.mode === "gate") { drawGate(); return; }
    ctx.clearRect(0, 0, W, H);
    if (game.mode === "menu") { drawMenu(); return; }
    for (var i = 0; i < game.poos.length; i++) drawPoo(game.poos[i].x, game.poos[i].y, game.poos[i].size);
    drawToilet(game.playerX);
    drawHud();
    if (game.mode === "over") drawGameOver();
  }

  function frame(time) {
    if (!lastTime) lastTime = time;
    var dt = Math.min(0.04, Math.max(0, (time - lastTime) / 1000));
    lastTime = time;
    update(dt);
    draw();
    (window.requestAnimationFrame || function (fn) { window.setTimeout(function () { fn(Date.now()); }, 16); })(frame);
  }

  window.addEventListener("resize", resizeStage);
  document.addEventListener("fullscreenchange", resizeStage);
  document.addEventListener("webkitfullscreenchange", resizeStage);
  resizeStage();
  canvas.focus();
  (window.requestAnimationFrame || function (fn) { window.setTimeout(function () { fn(Date.now()); }, 16); })(frame);
})();
