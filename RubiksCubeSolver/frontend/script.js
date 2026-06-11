const SOLVED_STATE =
    "WWWWWWWWW" +
    "RRRRRRRRR" +
    "GGGGGGGGG" +
    "YYYYYYYYY" +
    "OOOOOOOOO" +
    "BBBBBBBBB";

const FACE_ORDER = ["U", "R", "F", "D", "L", "B"];
const FACE_NAMES = {
    U: "Up",
    R: "Right",
    F: "Front",
    D: "Down",
    L: "Left",
    B: "Back",
};
const FACE_COLORS = {
    W: "#f8fafc",
    R: "#ef4444",
    G: "#22c55e",
    Y: "#facc15",
    O: "#f97316",
    B: "#3b82f6",
};
const COLOR_NAMES = {
    W: "White",
    R: "Red",
    G: "Green",
    Y: "Yellow",
    O: "Orange",
    B: "Blue",
};
const MOVES = ["U", "U'", "U2", "R", "R'", "R2", "F", "F'", "F2", "D", "D'", "D2", "L", "L'", "L2", "B", "B'", "B2"];
const CUBIE_GAP = 66;

const ALGORITHMS = [
    {
        name: "BFS",
        role: "Shortest path",
        time: "O(b^d)",
        space: "O(b^d)",
        text: "Uses a queue to explore cube states level by level. The first solved state found is the shortest within the selected depth.",
    },
    {
        name: "DFS",
        role: "Graph traversal",
        time: "O(b^m)",
        space: "O(bm)",
        text: "Uses a stack to go deep into one branch before backtracking to another branch. It is memory efficient but not always shortest.",
    },
    {
        name: "Backtracking",
        role: "Recursive undo",
        time: "O(b^d)",
        space: "O(d)",
        text: "Builds a solution recursively, rejects repeated states, and undoes invalid rotations before trying the next branch.",
    },
    {
        name: "Branch and Bound",
        role: "Optimization",
        time: "O(b^d)",
        space: "O(d)",
        text: "Maintains a best bound and prunes branches whose estimated cost cannot improve the current solution.",
    },
    {
        name: "Greedy",
        role: "Local choice",
        time: "O(bd)",
        space: "O(d)",
        text: "Chooses the rotation that locally reduces misplaced stickers the most. It is fast but can get stuck in local minima.",
    },
    {
        name: "Heuristic Search",
        role: "Best-first",
        time: "O(b^d)",
        space: "O(b^d)",
        text: "Prioritizes states with better heuristic scores so promising cube states are explored earlier.",
    },
    {
        name: "IDDFS",
        role: "Depth layers",
        time: "O(b^d)",
        space: "O(d)",
        text: "Runs depth-limited DFS repeatedly with increasing limits, combining BFS completeness with DFS memory usage.",
    },
    {
        name: "A*",
        role: "g(n) + h(n)",
        time: "O(b^d)",
        space: "O(b^d)",
        text: "Ranks states using path cost plus heuristic cost, making it a strong algorithm for guided optimal search.",
    },
];

let cubeState = SOLVED_STATE;
let selectedPaint = "W";
let moveHistory = [];
let solutionMoves = [];
let lastSolutionStart = SOLVED_STATE;
let isAnimating = false;

const cubeStage = document.querySelector("#cubeStage");
const cubeNet = document.querySelector("#cubeNet");
const palette = document.querySelector("#palette");
const validationStatus = document.querySelector("#validationStatus");
const backendStatus = document.querySelector("#backendStatus");
const solverMessage = document.querySelector("#solverMessage");
const cubeStateLabel = document.querySelector("#cubeStateLabel");
const historyList = document.querySelector("#historyList");
const solutionList = document.querySelector("#solutionList");
const comparisonChart = document.querySelector("#comparisonChart");
const complexityTable = document.querySelector("#complexityTable");
const algorithmGrid = document.querySelector("#algorithmGrid");
const moveCounter = document.querySelector("#moveCounter");
const timeTaken = document.querySelector("#timeTaken");
const algorithmUsed = document.querySelector("#algorithmUsed");
const nodesExpanded = document.querySelector("#nodesExpanded");

const algorithmSelect = document.querySelector("#algorithmSelect");
const depthInput = document.querySelector("#depthInput");
const shuffleBtn = document.querySelector("#shuffleBtn");
const solveBtn = document.querySelector("#solveBtn");
const resetBtn = document.querySelector("#resetBtn");
const randomBtn = document.querySelector("#randomBtn");
const validateBtn = document.querySelector("#validateBtn");
const playSolutionBtn = document.querySelector("#playSolutionBtn");

function apiUrl(path) {
    return window.location.protocol === "file:" ? `http://127.0.0.1:5000${path}` : path;
}

function sleep(ms) {
    return new Promise((resolve) => window.setTimeout(resolve, ms));
}

function vec(x, y, z) {
    return { x, y, z };
}

function indexToSticker(index, color) {
    const face = Math.floor(index / 9);
    const local = index % 9;
    const r = Math.floor(local / 3);
    const c = local % 3;

    if (face === 0) return { pos: vec(c - 1, 1, r - 1), normal: vec(0, 1, 0), color };
    if (face === 1) return { pos: vec(1, 1 - r, 1 - c), normal: vec(1, 0, 0), color };
    if (face === 2) return { pos: vec(c - 1, 1 - r, 1), normal: vec(0, 0, 1), color };
    if (face === 3) return { pos: vec(c - 1, -1, 1 - r), normal: vec(0, -1, 0), color };
    if (face === 4) return { pos: vec(-1, 1 - r, c - 1), normal: vec(-1, 0, 0), color };
    return { pos: vec(1 - c, 1 - r, -1), normal: vec(0, 0, -1), color };
}

function stickerToIndex(sticker) {
    const p = sticker.pos;
    const n = sticker.normal;
    let face = 0;
    let r = 0;
    let c = 0;

    if (n.y === 1) {
        face = 0;
        r = p.z + 1;
        c = p.x + 1;
    } else if (n.x === 1) {
        face = 1;
        r = 1 - p.y;
        c = 1 - p.z;
    } else if (n.z === 1) {
        face = 2;
        r = 1 - p.y;
        c = p.x + 1;
    } else if (n.y === -1) {
        face = 3;
        r = 1 - p.z;
        c = p.x + 1;
    } else if (n.x === -1) {
        face = 4;
        r = 1 - p.y;
        c = p.z + 1;
    } else {
        face = 5;
        r = 1 - p.y;
        c = 1 - p.x;
    }

    return face * 9 + r * 3 + c;
}

function stateToStickers(state) {
    return [...state].map((color, index) => indexToSticker(index, color));
}

function stickersToState(stickers) {
    const state = Array(54).fill("?");
    stickers.forEach((sticker) => {
        state[stickerToIndex(sticker)] = sticker.color;
    });
    return state.join("");
}

function rotateQuarter(v, axis, sign) {
    if (axis === "x") return sign > 0 ? vec(v.x, -v.z, v.y) : vec(v.x, v.z, -v.y);
    if (axis === "y") return sign > 0 ? vec(v.z, v.y, -v.x) : vec(-v.z, v.y, v.x);
    return sign > 0 ? vec(-v.y, v.x, v.z) : vec(v.y, -v.x, v.z);
}

function parseMove(move) {
    const face = move[0];
    const specs = {
        U: { axis: "y", layer: 1, sign: -1 },
        D: { axis: "y", layer: -1, sign: 1 },
        R: { axis: "x", layer: 1, sign: -1 },
        L: { axis: "x", layer: -1, sign: 1 },
        F: { axis: "z", layer: 1, sign: -1 },
        B: { axis: "z", layer: -1, sign: 1 },
    };
    const spec = { face, ...specs[face], turns: 1 };
    if (move.endsWith("'")) spec.sign *= -1;
    if (move.endsWith("2")) spec.turns = 2;
    return spec;
}

function layerCoordinate(pos, axis) {
    if (axis === "x") return pos.x;
    if (axis === "y") return pos.y;
    return pos.z;
}

function applyMove(state, move) {
    const spec = parseMove(move);
    let stickers = stateToStickers(state);
    for (let turn = 0; turn < spec.turns; turn += 1) {
        stickers = stickers.map((sticker) => {
            if (layerCoordinate(sticker.pos, spec.axis) !== spec.layer) return sticker;
            return {
                color: sticker.color,
                pos: rotateQuarter(sticker.pos, spec.axis, spec.sign),
                normal: rotateQuarter(sticker.normal, spec.axis, spec.sign),
            };
        });
    }
    return stickersToState(stickers);
}

function baseTransform(pos) {
    return `translate3d(${pos.x * CUBIE_GAP}px, ${-pos.y * CUBIE_GAP}px, ${pos.z * CUBIE_GAP}px)`;
}

function normalKey(normal) {
    if (normal.z === 1) return "zp";
    if (normal.z === -1) return "zn";
    if (normal.x === 1) return "xp";
    if (normal.x === -1) return "xn";
    if (normal.y === 1) return "yp";
    return "yn";
}

function positionKey(pos) {
    return `${pos.x},${pos.y},${pos.z}`;
}

function renderCube() {
    const cubies = new Map();
    stateToStickers(cubeState).forEach((sticker) => {
        const key = positionKey(sticker.pos);
        if (!cubies.has(key)) cubies.set(key, { pos: sticker.pos, stickers: {} });
        cubies.get(key).stickers[normalKey(sticker.normal)] = sticker.color;
    });

    const fragment = document.createDocumentFragment();
    cubies.forEach((cubie) => {
        const cubieEl = document.createElement("div");
        cubieEl.className = "cubie";
        cubieEl.dataset.x = cubie.pos.x;
        cubieEl.dataset.y = cubie.pos.y;
        cubieEl.dataset.z = cubie.pos.z;
        cubieEl.style.transform = baseTransform(cubie.pos);

        ["zp", "zn", "xp", "xn", "yp", "yn"].forEach((side) => {
            const faceEl = document.createElement("div");
            faceEl.className = `cubie-face side-${side}`;
            const color = cubie.stickers[side];
            if (color) {
                faceEl.style.background = FACE_COLORS[color];
                faceEl.dataset.color = color;
            }
            cubieEl.appendChild(faceEl);
        });
        fragment.appendChild(cubieEl);
    });

    cubeStage.replaceChildren(fragment);
}

function renderPalette() {
    const fragment = document.createDocumentFragment();
    Object.entries(FACE_COLORS).forEach(([key, color]) => {
        const button = document.createElement("button");
        button.className = `swatch ${selectedPaint === key ? "active" : ""}`;
        button.type = "button";
        button.title = COLOR_NAMES[key];
        button.style.background = color;
        button.addEventListener("click", () => {
            selectedPaint = key;
            renderPalette();
        });
        fragment.appendChild(button);
    });
    palette.replaceChildren(fragment);
}

function renderNet() {
    const fragment = document.createDocumentFragment();
    FACE_ORDER.forEach((face, faceIndex) => {
        const block = document.createElement("div");
        block.className = "face-block";
        block.dataset.face = face;

        const title = document.createElement("span");
        title.className = "face-title";
        title.textContent = `${face} - ${FACE_NAMES[face]}`;
        block.appendChild(title);

        const grid = document.createElement("div");
        grid.className = "face-grid";
        for (let i = 0; i < 9; i += 1) {
            const index = faceIndex * 9 + i;
            const sticker = document.createElement("button");
            sticker.className = `sticker ${i === 4 ? "locked" : ""}`;
            sticker.type = "button";
            sticker.title = `Sticker ${index + 1}`;
            sticker.style.background = FACE_COLORS[cubeState[index]];
            sticker.addEventListener("click", () => {
                if (i === 4 || isAnimating) return;
                cubeState = `${cubeState.slice(0, index)}${selectedPaint}${cubeState.slice(index + 1)}`;
                moveHistory = [];
                solutionMoves = [];
                lastSolutionStart = cubeState;
                cubeStateLabel.textContent = "Manual State";
                solverMessage.textContent = "Manual edit applied. Validate before solving.";
                updateStats();
                renderAll();
            });
            grid.appendChild(sticker);
        }
        block.appendChild(grid);
        fragment.appendChild(block);
    });
    cubeNet.replaceChildren(fragment);
}

function validateLocal() {
    const counts = { W: 0, R: 0, G: 0, Y: 0, O: 0, B: 0 };
    let message = "Cube state is valid.";
    let valid = cubeState.length === 54;

    [...cubeState].forEach((color) => {
        if (counts[color] === undefined) valid = false;
        else counts[color] += 1;
    });

    Object.keys(counts).forEach((color) => {
        if (counts[color] !== 9) {
            valid = false;
            message = `Invalid count: ${COLOR_NAMES[color]} has ${counts[color]} stickers.`;
        }
    });

    const centers = [
        [4, "W"],
        [13, "R"],
        [22, "G"],
        [31, "Y"],
        [40, "O"],
        [49, "B"],
    ];
    centers.forEach(([index, color]) => {
        if (cubeState[index] !== color) {
            valid = false;
            message = "Center stickers must remain W, R, G, Y, O, B.";
        }
    });

    if (valid) message = cubeState === SOLVED_STATE ? "Valid solved cube." : "Valid sticker counts. Ready for solver.";
    validationStatus.textContent = message;
    validationStatus.parentElement.classList.toggle("invalid", !valid);
    return { valid, message };
}

function renderHistoryList() {
    historyList.replaceChildren(...moveHistory.map((move, index) => {
        const item = document.createElement("li");
        item.textContent = move;
        return item;
    }));
}

function renderSolutionList(activeIndex = -1) {
    solutionList.replaceChildren(...solutionMoves.map((move, index) => {
        const item = document.createElement("li");
        item.textContent = move;
        if (index === activeIndex) item.classList.add("active-step");
        return item;
    }));
}

function renderLists(activeIndex = -1) {
    renderHistoryList();
    renderSolutionList(activeIndex);
}

function renderAlgorithmCards() {
    const cards = ALGORITHMS.map((algorithm) => {
        const card = document.createElement("article");
        card.className = "algorithm-card";
        card.innerHTML = `
            <strong>${algorithm.name}</strong>
            <h3>${algorithm.role}</h3>
            <p>${algorithm.text}</p>
        `;
        return card;
    });
    algorithmGrid.replaceChildren(...cards);
}

function renderComplexityTable() {
    const rows = ALGORITHMS.map((algorithm) => {
        const row = document.createElement("tr");
        row.innerHTML = `
            <td>${algorithm.name}</td>
            <td>${algorithm.time}</td>
            <td>${algorithm.space}</td>
        `;
        return row;
    });
    complexityTable.replaceChildren(...rows);
}

function renderComparisonChart(rows = []) {
    const data = rows.length ? rows : ALGORITHMS.map((item, index) => ({
        name: item.name,
        timeMs: (index + 1) * 0.5,
        nodes: (index + 1) * 120,
    }));
    const maxTime = Math.max(...data.map((row) => row.timeMs), 1);
    const bars = data.map((row) => {
        const width = Math.max(4, (row.timeMs / maxTime) * 100);
        const line = document.createElement("div");
        line.className = "bar-row";
        line.innerHTML = `
            <span class="bar-name">${row.name}</span>
            <span class="bar-track"><span class="bar-fill" style="width: ${width}%"></span></span>
            <span class="bar-value">${formatMs(row.timeMs)}</span>
        `;
        line.title = `${row.nodes.toLocaleString()} nodes`;
        return line;
    });
    comparisonChart.replaceChildren(...bars);
}

function renderAll(activeIndex = -1) {
    renderCube();
    renderNet();
    renderPalette();
    renderLists(activeIndex);
    validateLocal();
}

function formatMs(value) {
    if (!Number.isFinite(value)) return "0 ms";
    if (value < 1) return `${value.toFixed(2)} ms`;
    if (value < 1000) return `${value.toFixed(1)} ms`;
    return `${(value / 1000).toFixed(2)} s`;
}

function updateStats(data = null) {
    if (!data) {
        moveCounter.textContent = solutionMoves.length || moveHistory.length;
        timeTaken.textContent = "0 ms";
        algorithmUsed.textContent = algorithmSelect.options[algorithmSelect.selectedIndex].text;
        nodesExpanded.textContent = "0";
        return;
    }
    moveCounter.textContent = data.totalMoves ?? 0;
    timeTaken.textContent = formatMs(data.timeMs ?? 0);
    algorithmUsed.textContent = data.algorithm ?? algorithmSelect.value;
    nodesExpanded.textContent = Number(data.nodesExpanded ?? 0).toLocaleString();
}

function setBusy(busy) {
    isAnimating = busy;
    [shuffleBtn, solveBtn, resetBtn, randomBtn, validateBtn, playSolutionBtn, algorithmSelect, depthInput].forEach((element) => {
        element.disabled = busy;
    });
}

function randomMoves(length) {
    const moves = [];
    while (moves.length < length) {
        const move = MOVES[Math.floor(Math.random() * MOVES.length)];
        if (moves.length && moves[moves.length - 1][0] === move[0]) continue;
        moves.push(move);
    }
    return moves;
}

function rotationTransform(spec) {
    const degrees = spec.sign * spec.turns * 90;
    if (spec.axis === "x") return `rotateX(${degrees}deg)`;
    if (spec.axis === "y") return `rotateY(${degrees}deg)`;
    return `rotateZ(${degrees}deg)`;
}

async function animateMove(move, duration = 420) {
    const spec = parseMove(move);
    const affected = [...cubeStage.querySelectorAll(".cubie")].filter((cubie) => {
        const pos = {
            x: Number(cubie.dataset.x),
            y: Number(cubie.dataset.y),
            z: Number(cubie.dataset.z),
        };
        return layerCoordinate(pos, spec.axis) === spec.layer;
    });

    affected.forEach((cubie) => {
        const pos = {
            x: Number(cubie.dataset.x),
            y: Number(cubie.dataset.y),
            z: Number(cubie.dataset.z),
        };
        cubie.style.transition = `transform ${duration}ms cubic-bezier(.2,.8,.2,1)`;
        cubie.style.transform = `${rotationTransform(spec)} ${baseTransform(pos)}`;
    });

    await sleep(duration + 30);
    cubeState = applyMove(cubeState, move);
    renderCube();
    renderNet();
    validateLocal();
}

async function animateSequence(moves, keepSolutionList = false) {
    for (let index = 0; index < moves.length; index += 1) {
        renderSolutionList(keepSolutionList ? index : -1);
        solverMessage.textContent = `Applying move ${index + 1} of ${moves.length}: ${moves[index]}`;
        await animateMove(moves[index]);
    }
    renderSolutionList(-1);
}

async function scrambleCube(length, animated) {
    setBusy(true);
    cubeState = SOLVED_STATE;
    moveHistory = [];
    solutionMoves = [];
    lastSolutionStart = cubeState;
    renderAll();

    const moves = randomMoves(length);
    cubeStateLabel.textContent = animated ? "Animated Shuffle" : "Generated Random State";
    solverMessage.textContent = animated ? "Shuffling cube..." : "Random cube generated from legal rotations.";

    if (animated) {
        for (const move of moves) {
            await animateMove(move, 260);
            moveHistory.push(move);
            renderHistoryList();
        }
    } else {
        moves.forEach((move) => {
            cubeState = applyMove(cubeState, move);
        });
        moveHistory = moves;
        renderAll();
    }

    lastSolutionStart = cubeState;
    updateStats();
    solverMessage.textContent = `${length} legal rotations applied.`;
    setBusy(false);
}

function resetCube() {
    cubeState = SOLVED_STATE;
    moveHistory = [];
    solutionMoves = [];
    lastSolutionStart = SOLVED_STATE;
    cubeStateLabel.textContent = "Solved State";
    solverMessage.textContent = "Cube reset to solved state.";
    renderAll();
    updateStats();
}

async function solveCube() {
    const validation = validateLocal();
    if (!validation.valid) {
        solverMessage.textContent = validation.message;
        return;
    }

    setBusy(true);
    const startState = cubeState;
    solverMessage.textContent = "Sending cube state to C++ solver...";
    try {
        const response = await fetch(apiUrl("/api/solve"), {
            method: "POST",
            headers: { "Content-Type": "application/json" },
            body: JSON.stringify({
                state: cubeState,
                algorithm: algorithmSelect.value,
                history: moveHistory,
                maxDepth: Number(depthInput.value || 7),
                maxNodes: 150000,
            }),
        });
        const data = await response.json();
        if (!response.ok || data.error) {
            throw new Error(data.error || "Solver request failed.");
        }

        updateStats(data);
        renderComparisonChart(data.comparison || []);
        solverMessage.textContent = data.message || "Solution generated.";

        if (!data.solved) {
            solutionMoves = [];
            renderSolutionList();
            return;
        }

        solutionMoves = data.moves || [];
        lastSolutionStart = startState;
        renderSolutionList();

        if (solutionMoves.length) {
            setBusy(false);
            await playSolution();
            return;
        } else {
            cubeStateLabel.textContent = "Solved State";
        }
    } catch (error) {
        solverMessage.textContent = error.message;
    } finally {
        setBusy(false);
    }
}

async function playSolution() {
    if (!solutionMoves.length || isAnimating) return;
    setBusy(true);
    cubeState = lastSolutionStart;
    renderCube();
    renderNet();
    cubeStateLabel.textContent = "Solving Animation";
    await animateSequence(solutionMoves, true);
    cubeStateLabel.textContent = cubeState === SOLVED_STATE ? "Solved State" : "Solved Sequence Applied";
    moveHistory = [];
    renderLists();
    validateLocal();
    solverMessage.textContent = "Step-by-step solving animation completed.";
    setBusy(false);
}

async function checkBackend() {
    try {
        const response = await fetch(apiUrl("/api/health"));
        const data = await response.json();
        backendStatus.textContent = data.compiler ? "Backend online" : "Backend online, compiler missing";
        backendStatus.classList.toggle("online", Boolean(data.compiler));
        backendStatus.classList.toggle("offline", !data.compiler);
    } catch (error) {
        backendStatus.textContent = "Backend not running";
        backendStatus.classList.add("offline");
    }
}

shuffleBtn.addEventListener("click", () => scrambleCube(6, true));
randomBtn.addEventListener("click", () => scrambleCube(14, false));
resetBtn.addEventListener("click", resetCube);
solveBtn.addEventListener("click", solveCube);
validateBtn.addEventListener("click", validateLocal);
playSolutionBtn.addEventListener("click", playSolution);
algorithmSelect.addEventListener("change", () => updateStats());

renderAlgorithmCards();
renderComplexityTable();
renderComparisonChart();
renderAll();
updateStats();
checkBackend();
