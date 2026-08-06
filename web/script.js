const navToggle = document.getElementById("navToggle");
const primaryNav = document.getElementById("primaryNav");

navToggle.addEventListener("click", () => {
  const isOpen = primaryNav.classList.toggle("open");
  navToggle.setAttribute("aria-expanded", String(isOpen));
});

primaryNav.querySelectorAll("a").forEach((link) => {
  link.addEventListener("click", () => {
    primaryNav.classList.remove("open");
    navToggle.setAttribute("aria-expanded", "false");
  });
});

// Démo interactive : un mini moteur de dessin en canvas 2D. Aperçu
// simplifié pour le web — pas le moteur C++/Qt de l'application réelle.
(() => {
  const canvas = document.getElementById("demoCanvas");
  if (!canvas) {
    return;
  }
  const ctx = canvas.getContext("2d");
  const paletteEl = document.getElementById("demoPalette");
  const textInput = document.getElementById("demoTextInput");
  const shadowToggle = document.getElementById("demoShadow");
  const undoBtn = document.getElementById("demoUndo");
  const clearBtn = document.getElementById("demoClear");
  const toolButtons = document.querySelectorAll(".demo-tool[data-tool]");

  const PALETTE = [
    "#26a69a", "#1c2530", "#ffffff", "#e53935", "#43a047", "#1e88e5",
    "#fdd835", "#00acc1", "#d81b60", "#9e9e9e", "#fb8c00", "#8e24aa", "#6d4c41",
  ];
  const HANDLE_TOLERANCE = 10;
  const MIN_SIZE = 10;

  let shapes = [];
  let nextId = 1;
  let selectedId = null;
  let activeTool = "select";
  let activeColor = PALETTE[0];
  let shadowEnabled = false;
  let drag = null;
  let pendingTextPos = null;
  let history = [];

  function seed() {
    shapes.push({ id: nextId++, type: "rect", x: 90, y: 80, w: 170, h: 120, color: "#26a69a", shadow: true });
    shapes.push({ id: nextId++, type: "ellipse", x: 240, y: 150, w: 140, h: 140, color: "#ff7043", shadow: true });
  }

  function pushHistory() {
    history.push(JSON.parse(JSON.stringify(shapes)));
    if (history.length > 50) {
      history.shift();
    }
  }

  function undo() {
    if (!history.length) {
      return;
    }
    shapes = history.pop();
    selectedId = null;
    render();
  }

  function getPos(evt) {
    const rect = canvas.getBoundingClientRect();
    const scaleX = canvas.width / rect.width;
    const scaleY = canvas.height / rect.height;
    return {
      x: (evt.clientX - rect.left) * scaleX,
      y: (evt.clientY - rect.top) * scaleY,
    };
  }

  function textMetrics(shape) {
    ctx.font = "600 24px -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif";
    const m = ctx.measureText(shape.text);
    const ascent = m.actualBoundingBoxAscent || 19;
    const descent = m.actualBoundingBoxDescent || 6;
    return { w: m.width, ascent, descent };
  }

  function shapeBounds(shape) {
    if (shape.type === "text") {
      const m = textMetrics(shape);
      return { x: shape.x, y: shape.y - m.ascent, w: m.w, h: m.ascent + m.descent };
    }
    let { x, y, w, h } = shape;
    if (w < 0) {
      x += w;
      w = -w;
    }
    if (h < 0) {
      y += h;
      h = -h;
    }
    return { x, y, w, h };
  }

  function corners(b) {
    return [
      { name: "nw", x: b.x, y: b.y },
      { name: "ne", x: b.x + b.w, y: b.y },
      { name: "se", x: b.x + b.w, y: b.y + b.h },
      { name: "sw", x: b.x, y: b.y + b.h },
    ];
  }

  function hitTestHandle(shape, pos) {
    if (shape.type === "text") {
      return null;
    }
    const b = shapeBounds(shape);
    return corners(b).find((c) => Math.hypot(c.x - pos.x, c.y - pos.y) <= HANDLE_TOLERANCE) || null;
  }

  function hitTestShape(pos) {
    for (let i = shapes.length - 1; i >= 0; i -= 1) {
      const shape = shapes[i];
      const b = shapeBounds(shape);
      if (shape.type === "ellipse") {
        const rx = Math.max(b.w / 2, 0.01);
        const ry = Math.max(b.h / 2, 0.01);
        const cx = b.x + b.w / 2;
        const cy = b.y + b.h / 2;
        const dx = (pos.x - cx) / rx;
        const dy = (pos.y - cy) / ry;
        if (dx * dx + dy * dy <= 1) {
          return shape;
        }
      } else if (pos.x >= b.x && pos.x <= b.x + b.w && pos.y >= b.y && pos.y <= b.y + b.h) {
        return shape;
      }
    }
    return null;
  }

  function resizeShape(shape, dragState, pos) {
    const o = dragState.orig;
    const dx = pos.x - dragState.start.x;
    const dy = pos.y - dragState.start.y;
    let { x, y, w, h } = o;
    if (dragState.handle === "se" || dragState.handle === "ne") {
      w = Math.max(MIN_SIZE, o.w + dx);
    } else {
      w = Math.max(MIN_SIZE, o.w - dx);
      x = o.x + o.w - w;
    }
    if (dragState.handle === "se" || dragState.handle === "sw") {
      h = Math.max(MIN_SIZE, o.h + dy);
    } else {
      h = Math.max(MIN_SIZE, o.h - dy);
      y = o.y + o.h - h;
    }
    shape.x = x;
    shape.y = y;
    shape.w = w;
    shape.h = h;
  }

  function drawShape(shape) {
    ctx.save();
    if (shape.shadow) {
      ctx.shadowColor = "rgba(15, 23, 32, 0.35)";
      ctx.shadowBlur = 10;
      ctx.shadowOffsetX = 6;
      ctx.shadowOffsetY = 6;
    }
    const b = shapeBounds(shape);
    if (shape.type === "rect") {
      ctx.fillStyle = shape.color;
      ctx.fillRect(b.x, b.y, b.w, b.h);
    } else if (shape.type === "ellipse") {
      ctx.beginPath();
      ctx.ellipse(b.x + b.w / 2, b.y + b.h / 2, Math.max(b.w / 2, 0.01), Math.max(b.h / 2, 0.01), 0, 0, Math.PI * 2);
      ctx.fillStyle = shape.color;
      ctx.fill();
    } else if (shape.type === "text") {
      ctx.fillStyle = shape.color;
      ctx.font = "600 24px -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif";
      ctx.textBaseline = "alphabetic";
      ctx.fillText(shape.text, shape.x, shape.y);
    }
    ctx.restore();
  }

  function drawSelection(shape) {
    const b = shapeBounds(shape);
    ctx.save();
    ctx.strokeStyle = "#1e88e5";
    ctx.setLineDash([4, 3]);
    ctx.lineWidth = 1.5;
    ctx.strokeRect(b.x - 3, b.y - 3, b.w + 6, b.h + 6);
    ctx.setLineDash([]);
    if (shape.type !== "text") {
      ctx.fillStyle = "#1e88e5";
      corners(b).forEach((c) => ctx.fillRect(c.x - 4, c.y - 4, 8, 8));
    }
    ctx.restore();
  }

  function render() {
    ctx.clearRect(0, 0, canvas.width, canvas.height);
    ctx.fillStyle = "#ffffff";
    ctx.fillRect(0, 0, canvas.width, canvas.height);
    shapes.forEach(drawShape);
    const selected = shapes.find((s) => s.id === selectedId);
    if (selected) {
      drawSelection(selected);
    }
  }

  function setTool(tool) {
    activeTool = tool;
    toolButtons.forEach((btn) => btn.classList.toggle("is-active", btn.dataset.tool === tool));
    if (tool !== "select") {
      selectedId = null;
    }
    render();
  }

  function openTextInput(pos) {
    const rect = canvas.getBoundingClientRect();
    const scaleX = rect.width / canvas.width;
    const scaleY = rect.height / canvas.height;
    textInput.style.left = `${pos.x * scaleX}px`;
    textInput.style.top = `${pos.y * scaleY - 22}px`;
    textInput.style.display = "block";
    textInput.value = "";
    pendingTextPos = pos;
    textInput.focus();
  }

  function commitTextInput() {
    const value = textInput.value.trim();
    textInput.style.display = "none";
    if (value && pendingTextPos) {
      pushHistory();
      const shape = {
        id: nextId++, type: "text", x: pendingTextPos.x, y: pendingTextPos.y,
        color: activeColor, text: value, shadow: shadowEnabled,
      };
      shapes.push(shape);
      selectedId = shape.id;
      render();
    }
    pendingTextPos = null;
  }

  textInput.addEventListener("keydown", (e) => {
    if (e.key === "Enter") {
      e.preventDefault();
      commitTextInput();
    } else if (e.key === "Escape") {
      e.preventDefault();
      textInput.style.display = "none";
      pendingTextPos = null;
    }
  });
  textInput.addEventListener("blur", commitTextInput);

  canvas.addEventListener("pointerdown", (e) => {
    // Empêche le focus par défaut du navigateur sur le canvas, qui sinon
    // reprend la main après le focus programmatique posé sur le champ de
    // texte ci-dessous et le fait perdre aussitôt (blur avec valeur vide).
    e.preventDefault();
    canvas.focus();
    const pos = getPos(e);

    if (activeTool === "text") {
      openTextInput(pos);
      return;
    }

    if (activeTool === "select") {
      const selected = shapes.find((s) => s.id === selectedId);
      if (selected) {
        const handle = hitTestHandle(selected, pos);
        if (handle) {
          pushHistory();
          canvas.setPointerCapture(e.pointerId);
          drag = { mode: "resize", id: selected.id, handle: handle.name, start: pos, orig: shapeBounds(selected) };
          return;
        }
      }
      const hit = hitTestShape(pos);
      if (hit) {
        selectedId = hit.id;
        pushHistory();
        canvas.setPointerCapture(e.pointerId);
        drag = { mode: "move", id: hit.id, last: pos };
      } else {
        selectedId = null;
      }
      render();
      return;
    }

    pushHistory();
    const shape = { id: nextId++, type: activeTool, x: pos.x, y: pos.y, w: 0, h: 0, color: activeColor, shadow: shadowEnabled };
    shapes.push(shape);
    selectedId = shape.id;
    canvas.setPointerCapture(e.pointerId);
    drag = { mode: "create", id: shape.id, start: pos };
    render();
  });

  canvas.addEventListener("pointermove", (e) => {
    if (!drag) {
      return;
    }
    const pos = getPos(e);
    const shape = shapes.find((s) => s.id === drag.id);
    if (!shape) {
      return;
    }
    if (drag.mode === "create") {
      shape.w = pos.x - drag.start.x;
      shape.h = pos.y - drag.start.y;
    } else if (drag.mode === "move") {
      shape.x += pos.x - drag.last.x;
      shape.y += pos.y - drag.last.y;
      drag.last = pos;
    } else if (drag.mode === "resize") {
      resizeShape(shape, drag, pos);
    }
    render();
  });

  function endDrag() {
    drag = null;
  }
  canvas.addEventListener("pointerup", endDrag);
  canvas.addEventListener("pointercancel", endDrag);

  canvas.addEventListener("keydown", (e) => {
    if ((e.key === "Delete" || e.key === "Backspace") && selectedId != null) {
      e.preventDefault();
      pushHistory();
      shapes = shapes.filter((s) => s.id !== selectedId);
      selectedId = null;
      render();
    } else if (e.key.toLowerCase() === "z" && (e.ctrlKey || e.metaKey)) {
      e.preventDefault();
      undo();
    }
  });

  toolButtons.forEach((btn) => {
    btn.addEventListener("click", () => setTool(btn.dataset.tool));
  });

  PALETTE.forEach((color, index) => {
    const swatch = document.createElement("button");
    swatch.type = "button";
    swatch.className = "demo-swatch";
    if (index === 0) {
      swatch.classList.add("is-active");
    }
    swatch.style.background = color;
    swatch.setAttribute("aria-label", `Couleur ${color}`);
    swatch.addEventListener("click", () => {
      activeColor = color;
      paletteEl.querySelectorAll(".demo-swatch").forEach((s) => s.classList.remove("is-active"));
      swatch.classList.add("is-active");
      const selected = shapes.find((s) => s.id === selectedId);
      if (selected) {
        pushHistory();
        selected.color = color;
        render();
      }
    });
    paletteEl.appendChild(swatch);
  });

  shadowToggle.addEventListener("change", () => {
    shadowEnabled = shadowToggle.checked;
    const selected = shapes.find((s) => s.id === selectedId);
    if (selected) {
      pushHistory();
      selected.shadow = shadowEnabled;
      render();
    }
  });

  undoBtn.addEventListener("click", undo);
  clearBtn.addEventListener("click", () => {
    if (!shapes.length) {
      return;
    }
    pushHistory();
    shapes = [];
    selectedId = null;
    render();
  });

  seed();
  render();
})();
