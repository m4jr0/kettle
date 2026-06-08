// Copyright 2026 m4jr0. All Rights Reserved.
// Use of this source code is governed by the MIT
// license that can be found in the LICENSE file.

const graph = new LGraph();
const canvas = new LGraphCanvas("#graph-canvas", graph);

// -----------------------------------------------------------------------------
// Kettle types
// -----------------------------------------------------------------------------

const KETTLE_TYPES = Object.freeze({
    NONE: "none",
    BOOL: "bool",

    INT8: "int8",
    INT16: "int16",
    INT32: "int32",
    INT64: "int64",

    UINT8: "uint8",
    UINT16: "uint16",
    UINT32: "uint32",
    UINT64: "uint64",

    FLOAT32: "float32",
    FLOAT64: "float64",

    STRING: "string",
    OBJECT: "object",

    VEC2: "vec2",
    VEC3: "vec3",
    VEC4: "vec4",

    BLOB: "blob",

    EXEC: "exec",
    ANY: "any",
});

const KETTLE_NODE_TYPES = Object.freeze({
    EVENT_BEGIN: "EventBegin",
    GET_PLAYER: "GetPlayer",
    GET_HEALTH: "GetHealth",
    CONSTANT: "Constant",
    COMPARE: "Compare",
    BRANCH: "Branch",
    WHILE: "While",
    PRINT: "Print",
    SET_POSITION: "SetPosition",
});

const VALUE_TYPES = [
    KETTLE_TYPES.BOOL,
    KETTLE_TYPES.INT8,
    KETTLE_TYPES.UINT8,
    KETTLE_TYPES.INT16,
    KETTLE_TYPES.UINT16,
    KETTLE_TYPES.INT32,
    KETTLE_TYPES.UINT32,
    KETTLE_TYPES.INT64,
    KETTLE_TYPES.UINT64,
    KETTLE_TYPES.FLOAT32,
    KETTLE_TYPES.FLOAT64,
    KETTLE_TYPES.STRING,
    KETTLE_TYPES.OBJECT,
    KETTLE_TYPES.VEC2,
    KETTLE_TYPES.VEC3,
    KETTLE_TYPES.VEC4,
    KETTLE_TYPES.BLOB,
];
const NODE_PROPERTIES = Object.freeze({
    VALUE_TYPE: "valueType",
    OP: "op",
});

const COMPARABLE_TYPES = [
    KETTLE_TYPES.BOOL,
    KETTLE_TYPES.INT8,
    KETTLE_TYPES.UINT8,
    KETTLE_TYPES.INT16,
    KETTLE_TYPES.UINT16,
    KETTLE_TYPES.INT32,
    KETTLE_TYPES.UINT32,
    KETTLE_TYPES.INT64,
    KETTLE_TYPES.UINT64,
    KETTLE_TYPES.FLOAT32,
    KETTLE_TYPES.FLOAT64,
    KETTLE_TYPES.STRING,
    KETTLE_TYPES.OBJECT,
    KETTLE_TYPES.VEC2,
    KETTLE_TYPES.VEC3,
    KETTLE_TYPES.VEC4,
];

const COMPARISON_OPS = ["==", "!=", "<", "<=", ">", ">="];
const ORDER_COMPARISON_OPS = ["==", "!=", "<", "<=", ">", ">="];
const EQUALITY_COMPARISON_OPS = ["==", "!="];

function getCompareOpsForType(type) {
    if (isIntegerType(type) || isFloatType(type)) {
        return ORDER_COMPARISON_OPS;
    }

    if (
        type === KETTLE_TYPES.BOOL ||
        type === KETTLE_TYPES.STRING ||
        type === KETTLE_TYPES.OBJECT ||
        type === KETTLE_TYPES.VEC2 ||
        type === KETTLE_TYPES.VEC3 ||
        type === KETTLE_TYPES.VEC4
    ) {
        return EQUALITY_COMPARISON_OPS;
    }

    return [];
}

function normalizeCompareOpForType(type, op) {
    const ops = getCompareOpsForType(type);

    if (ops.includes(op)) {
        return op;
    }

    if (ops.length === 0) {
        return "";
    }

    return ops[0];
}

// -----------------------------------------------------------------------------
// Sanitizer
// -----------------------------------------------------------------------------

function sanitizeGraph() {
    const lastInputLink = new Map();
    const lastOutputLink = new Map();

    // First pass: record the last link for each slot.
    for (const link of Object.values(graph.links || {})) {
        const from = graph.getNodeById(link.origin_id);
        const to = graph.getNodeById(link.target_id);

        if (!from || !to) continue;

        const inputKey = `${to.id}:${link.target_slot}`;
        lastInputLink.set(inputKey, link);
        const out = from.outputs?.[link.origin_slot];

        if (out?.type === KETTLE_TYPES.EXEC) {
            const outputKey = `${from.id}:${link.origin_slot}`;
            lastOutputLink.set(outputKey, link);
        }
    }

    // Second pass: remove all links that are not the last one.
    for (const link of Object.values(graph.links || {})) {
        const from = graph.getNodeById(link.origin_id);
        const to = graph.getNodeById(link.target_id);

        if (!from || !to) continue;

        const inputKey = `${to.id}:${link.target_slot}`;

        if (lastInputLink.get(inputKey)?.id !== link.id) {
            graph.removeLink(link.id);
            continue;
        }

        const out = from.outputs?.[link.origin_slot];

        if (out?.type === KETTLE_TYPES.EXEC) {
            const outputKey = `${from.id}:${link.origin_slot}`;

            if (lastOutputLink.get(outputKey)?.id !== link.id) {
                graph.removeLink(link.id);
                continue;
            }
        }
    }
}

// -----------------------------------------------------------------------------
// Node definitions
// -----------------------------------------------------------------------------

const NODE_DEFS = {
    [KETTLE_NODE_TYPES.EVENT_BEGIN]: {
        outputs: [["exec", KETTLE_TYPES.EXEC]],
        properties: {},
    },

    [KETTLE_NODE_TYPES.GET_PLAYER]: {
        inputs: [["exec", KETTLE_TYPES.EXEC]],
        outputs: [
            ["then", KETTLE_TYPES.EXEC],
            ["player", KETTLE_TYPES.OBJECT],
        ],
        properties: {},
    },

    [KETTLE_NODE_TYPES.GET_HEALTH]: {
        inputs: [
            ["exec", KETTLE_TYPES.EXEC],
            ["player", KETTLE_TYPES.OBJECT],
        ],
        outputs: [
            ["then", KETTLE_TYPES.EXEC],
            ["health", KETTLE_TYPES.FLOAT32],
        ],
        properties: {},
    },

    [KETTLE_NODE_TYPES.CONSTANT]: {
        outputs: [["value", KETTLE_TYPES.INT32]],
        properties: {
            valueType: { type: KETTLE_TYPES.STRING, default: KETTLE_TYPES.INT32 },
            value: { type: KETTLE_TYPES.INT32, default: 0 },
        },
        onCreate(node) {
            addValueTypeWidget(node, NODE_PROPERTIES.VALUE_TYPE, newType => {
                node.properties[NODE_PROPERTIES.VALUE_TYPE] = newType;
                node.properties.value = defaultValueForType(newType);
                setOutputType(node, "value", newType);

                rebuildNodeWidgets(node);
            });
        },
    },

    [KETTLE_NODE_TYPES.COMPARE]: {
        inputs: [
            ["exec", KETTLE_TYPES.EXEC],
            ["a", KETTLE_TYPES.FLOAT32],
            ["b", KETTLE_TYPES.FLOAT32],
        ],
        outputs: [
            ["then", KETTLE_TYPES.EXEC],
            ["result", KETTLE_TYPES.BOOL],
        ],
        properties: {
            valueType: { type: KETTLE_TYPES.STRING, default: KETTLE_TYPES.FLOAT32 },
            op: { type: KETTLE_TYPES.STRING, default: COMPARISON_OPS[0] },
        },
        onCreate(node) {
            addValueTypeWidget(node, NODE_PROPERTIES.VALUE_TYPE, newType => {
                node.properties[NODE_PROPERTIES.VALUE_TYPE] = newType;
                node.properties[NODE_PROPERTIES.OP] = normalizeCompareOpForType(
                    newType,
                    node.properties[NODE_PROPERTIES.OP]
                );

                setInputType(node, "a", newType);
                setInputType(node, "b", newType);

                rebuildNodeWidgets(node);
            }, COMPARABLE_TYPES);

            const valueType = node.properties[NODE_PROPERTIES.VALUE_TYPE];
            const ops = getCompareOpsForType(valueType);

            node.properties[NODE_PROPERTIES.OP] = normalizeCompareOpForType(
                valueType,
                node.properties[NODE_PROPERTIES.OP]
            );

            if (ops.length > 0) {
                node.addWidget("combo", NODE_PROPERTIES.OP, node.properties[NODE_PROPERTIES.OP], v => {
                    node.properties[NODE_PROPERTIES.OP] = String(v);
                }, { values: ops });
            }
        },
    },

    [KETTLE_NODE_TYPES.BRANCH]: {
        inputs: [
            ["exec", KETTLE_TYPES.EXEC],
            ["condition", KETTLE_TYPES.BOOL],
        ],
        outputs: [
            ["true", KETTLE_TYPES.EXEC],
            ["false", KETTLE_TYPES.EXEC],
        ],
        properties: {},
    },

    [KETTLE_NODE_TYPES.WHILE]: {
        inputs: [
            ["exec", KETTLE_TYPES.EXEC],
            ["condition", KETTLE_TYPES.BOOL],
        ],
        outputs: [
            ["body", KETTLE_TYPES.EXEC],
            ["then", KETTLE_TYPES.EXEC],
        ],
        properties: {},
    },

    [KETTLE_NODE_TYPES.PRINT]: {
        inputs: [
            ["exec", KETTLE_TYPES.EXEC],
            ["value", KETTLE_TYPES.STRING],
        ],
        outputs: [["then", KETTLE_TYPES.EXEC]],
        properties: {
            valueType: { type: KETTLE_TYPES.STRING, default: KETTLE_TYPES.STRING },
            message: { type: KETTLE_TYPES.STRING, default: "" },
        },
        onCreate(node) {
            addValueTypeWidget(node, NODE_PROPERTIES.VALUE_TYPE, newType => {
                node.properties[NODE_PROPERTIES.VALUE_TYPE] = newType;
                setInputType(node, "value", newType);
            });
        },
    },

    [KETTLE_NODE_TYPES.SET_POSITION]: {
        inputs: [
            ["exec", KETTLE_TYPES.EXEC],
            ["target", KETTLE_TYPES.OBJECT],
            ["position", KETTLE_TYPES.VEC3],
        ],
        outputs: [["then", KETTLE_TYPES.EXEC]],
        properties: {
            position: { type: KETTLE_TYPES.VEC3, default: [0.0, 0.0, 0.0] },
        },
    },
};

// -----------------------------------------------------------------------------
// Spawn layout
// -----------------------------------------------------------------------------

const spawnXStart = 120;
const spawnYStart = 80;
const spawnStepX = 220;
const spawnStepY = 140;

let nextId = 1;
let spawnX = spawnXStart;
let spawnY = spawnYStart;

let spawnWrapX = 1200;
let spawnWrapY = 700;

resizeCanvas();
window.addEventListener("resize", resizeCanvas);

function resizeCanvas() {
    const c = document.getElementById("graph-canvas");
    const rect = c.getBoundingClientRect();
    const dpr = window.devicePixelRatio || 1;

    c.width = Math.floor(rect.width * dpr);
    c.height = Math.floor(rect.height * dpr);

    spawnWrapX = rect.width;
    spawnWrapY = rect.height;

    canvas.ds.scale = 1;
    canvas.draw(true, true);
}

function getNextNodePosition(node) {
    node.computeSize?.();

    const width = node.size?.[0] ?? 180;
    const height = node.size?.[1] ?? 80;

    const position = [spawnX, spawnY];

    spawnX += Math.max(spawnStepX, width + 60);

    if (spawnX + width > spawnWrapX) {
        spawnX = spawnXStart;
        spawnY += Math.max(spawnStepY, height + 60);
    }

    if (spawnY + height > spawnWrapY) {
        spawnY = spawnYStart;
    }

    return position;
}

// -----------------------------------------------------------------------------
// Node registration
// -----------------------------------------------------------------------------

function registerAllNodeTypes() {
    for (const [type, def] of Object.entries(NODE_DEFS)) {
        registerNodeType(type, def);
    }
}

function registerNodeType(type, def) {
    function KettleNode() {
        this.title = type;
        this.kettleType = type;

        for (const [name, pinType] of def.inputs || []) {
            this.addInput(name, pinType);
        }

        for (const [name, pinType] of def.outputs || []) {
            this.addOutput(name, pinType);
        }

        this.properties = {};

        for (const [key, prop] of Object.entries(def.properties || {})) {
            this.properties[key] = cloneValue(prop.default);
        }

        rebuildNodeWidgets(this);
    }

    KettleNode.prototype.onConnectionsChange = function (type, slot, connected, linkInfo) {
        if (!connected || !linkInfo) return;

        if (type === LiteGraph.INPUT) {
            const input = this.inputs?.[slot];
            if (!input) return;

            // Newest input takes precedence.
            for (const link of Object.values(graph.links || {})) {
                if (
                    link.target_id === this.id &&
                    link.target_slot === slot &&
                    link.id !== input.link
                ) {
                    graph.removeLink(link.id);
                }
            }
        }

        if (type === LiteGraph.OUTPUT) {
            const output = this.outputs?.[slot];
            if (!output || output.type !== KETTLE_TYPES.EXEC) return;

            // Newest output takes precedence.
            for (const linkId of [...(output.links || [])]) {
                if (linkId !== linkInfo.id) {
                    graph.removeLink(linkId);
                }
            }
        }
    };

    KettleNode.title = type;
    LiteGraph.registerNodeType(`kettle/${type}`, KettleNode);
}

function addDynamicHeaderWidgets(node) {
    const def = NODE_DEFS[node.kettleType || node.title];
    if (!def?.onCreate) return;
    def.onCreate(node);
}

function isDynamicHeaderWidget(propKey) {
    return propKey !== NODE_PROPERTIES.VALUE_TYPE && propKey !== NODE_PROPERTIES.OP;
}

function rebuildNodeWidgets(node) {
    node.widgets = [];

    addDynamicHeaderWidgets(node); // Selectors first.

    const def = NODE_DEFS[node.kettleType || node.title];

    if (!def) return;

    for (const [key, prop] of Object.entries(def.properties || {})) {
        if (!isDynamicHeaderWidget(key)) continue;
        addPropertyWidget(node, key, prop);
    }

    node.computeSize?.();
}

function addPropertyWidget(node, key, prop) {
    const type = getEffectivePropertyType(node, key, prop);

    if (isIntegerType(type) || isFloatType(type)) {
        node.addWidget("number", key, Number(node.properties[key] ?? 0), v => {
            node.properties[key] = Number(v);
        });
        return;
    }

    if (type === KETTLE_TYPES.BOOL) {
        node.addWidget("toggle", key, Boolean(node.properties[key]), v => {
            node.properties[key] = Boolean(v);
        });
        return;
    }

    if (type === KETTLE_TYPES.STRING) {
        node.addWidget("text", key, String(node.properties[key] ?? ""), v => {
            node.properties[key] = String(v);
        });
        return;
    }

    if (type === KETTLE_TYPES.VEC2) {
        ensureVectorProperty(node, key, 2);
        addVectorWidgets(node, key, ["x", "y"]);
        return;
    }

    if (type === KETTLE_TYPES.VEC3) {
        ensureVectorProperty(node, key, 3);
        addVectorWidgets(node, key, ["x", "y", "z"]);
        return;
    }

    if (type === KETTLE_TYPES.VEC4) {
        ensureVectorProperty(node, key, 4);
        addVectorWidgets(node, key, ["x", "y", "z", "w"]);
        return;
    }

    if (type === KETTLE_TYPES.OBJECT) {
        let handle = node.properties[key] ?? { index: 0, generation: 0 };
        node.properties[key] = handle;

        node.addWidget("number", `${key}.index`, handle.index ?? 0, v => {
            node.properties[key].index = Number(v);
        });

        node.addWidget("number", `${key}.generation`, handle.generation ?? 0, v => {
            node.properties[key].generation = Number(v);
        });

        return;
    }

    if (type === KETTLE_TYPES.BLOB) {
        node.addWidget("text", key, JSON.stringify(node.properties[key] ?? {}), v => {
            try {
                node.properties[key] = JSON.parse(v);
            } catch {
                node.properties[key] = null;
            }
        });
    }
}

function addValueTypeWidget(node, key, onChange, values = VALUE_TYPES) {
    node.addWidget("combo", key, node.properties[key], v => {
        onChange(String(v));
    }, {
        values
    });
}

function addVectorWidgets(node, key, names) {
    for (let i = 0; i < names.length; ++i) {
        node.addWidget("number", `${key}.${names[i]}`, node.properties[key][i], v => {
            node.properties[key][i] = Number(v);
        });
    }
}

function ensureVectorProperty(node, key, count, defaultValue = 0.0) {
    if (!Array.isArray(node.properties[key])) {
        node.properties[key] = new Array(count).fill(defaultValue);
    }

    while (node.properties[key].length < count) {
        node.properties[key].push(defaultValue);
    }

    node.properties[key] = node.properties[key].slice(0, count);
}

// -----------------------------------------------------------------------------
// Graph operations
// -----------------------------------------------------------------------------

function addNode(type, pos = null) {
    const node = LiteGraph.createNode(`kettle/${type}`);

    if (!node) {
        console.error("Unknown node type:", type);
        return null;
    }

    node.id = nextId++;
    node.pos = pos || getNextNodePosition(node);

    graph.add(node);

    return node;
}

function exportKettleJSON() {
    const nodes = [];
    const links = [];

    graph._nodes.forEach(node => {
        const pins = [];

        node.inputs?.forEach(pin => {
            pins.push({
                name: pin.name,
                type: pin.type || KETTLE_TYPES.ANY,
                direction: "input",
            });
        });

        node.outputs?.forEach(pin => {
            pins.push({
                name: pin.name,
                type: pin.type || KETTLE_TYPES.ANY,
                direction: "output",
            });
        });

        nodes.push({
            id: node.id,
            type: node.kettleType || node.title,
            name: node.title,
            position: [node.pos[0], node.pos[1]],
            pins,
            properties: convertProperties(node),
        });
    });

    Object.values(graph.links || {}).forEach(link => {
        const from = graph.getNodeById(link.origin_id);
        const to = graph.getNodeById(link.target_id);

        if (!from || !to) return;

        links.push({
            from_node: from.id,
            from_pin: from.outputs[link.origin_slot].name,
            to_node: to.id,
            to_pin: to.inputs[link.target_slot].name,
        });
    });

    return {
        version: 1,
        nodes,
        links,
    };
}

function importKettleJSON(json) {
    graph.clear();
    nextId = 1;

    const idToNode = new Map();

    for (const src of json.nodes || []) {
        const node = LiteGraph.createNode(`kettle/${src.type}`);

        if (!node) {
            console.warn("Unknown node type during import:", src.type);
            continue;
        }

        node.id = src.id;
        node.title = src.name || src.type;
        node.kettleType = src.type;
        node.pos = src.position || getNextNodePosition(node);

        const def = NODE_DEFS[src.type];
        node.properties = {};

        for (const [key, propDef] of Object.entries(def?.properties || {})) {
            node.properties[key] = cloneValue(propDef.default);
        }

        for (const [key, prop] of Object.entries(src.properties || {})) {
            node.properties[key] = cloneValue(prop.value);
        }

        applyDynamicNodeShape(node);
        rebuildNodeWidgets(node);

        graph.add(node);
        idToNode.set(src.id, node);
        nextId = Math.max(nextId, src.id + 1);
    }

    for (const link of json.links || []) {
        const from = idToNode.get(link.from_node);
        const to = idToNode.get(link.to_node);

        if (!from || !to) continue;

        const fromSlot = findOutputSlot(from, link.from_pin);
        const toSlot = findInputSlot(to, link.to_pin);

        if (fromSlot < 0 || toSlot < 0) continue;

        from.connect(fromSlot, to, toSlot);
    }

    sanitizeGraph();
}

function applyDynamicNodeShape(node) {
    if (node.kettleType === KETTLE_NODE_TYPES.CONSTANT) {
        const type = node.properties[NODE_PROPERTIES.VALUE_TYPE] || inferPropertyType(node.properties.value);
        node.properties[NODE_PROPERTIES.VALUE_TYPE] = type;
        setOutputType(node, "value", type);
        return;
    }

    if (node.kettleType === KETTLE_NODE_TYPES.COMPARE) {
        const type = node.properties[NODE_PROPERTIES.VALUE_TYPE] || KETTLE_TYPES.INT32;
        node.properties[NODE_PROPERTIES.VALUE_TYPE] = type;
        setInputType(node, "a", type);
        setInputType(node, "b", type);
        return;
    }

    if (node.kettleType === KETTLE_NODE_TYPES.PRINT) {
        const type = node.properties[NODE_PROPERTIES.VALUE_TYPE] || KETTLE_TYPES.STRING;
        node.properties[NODE_PROPERTIES.VALUE_TYPE] = type;
        setInputType(node, "value", type);
        return;
    }
}

// -----------------------------------------------------------------------------
// Pin helpers
// -----------------------------------------------------------------------------

function findOutputSlot(node, name) {
    return (node.outputs || []).findIndex(pin => pin.name === name);
}

function findInputSlot(node, name) {
    return (node.inputs || []).findIndex(pin => pin.name === name);
}

function setInputType(node, pinName, type) {
    const index = findInputSlot(node, pinName);

    if (index >= 0) {
        node.inputs[index].type = type;
    }
}

function setOutputType(node, pinName, type) {
    const index = findOutputSlot(node, pinName);

    if (index >= 0) {
        node.outputs[index].type = type;
    }
}

// -----------------------------------------------------------------------------
// Property conversion
// -----------------------------------------------------------------------------

function convertProperties(node) {
    const result = {};
    const def = NODE_DEFS[node.kettleType || node.title];

    for (const [key, value] of Object.entries(node.properties || {})) {
        const propDef = def?.properties?.[key];

        if (propDef?.hidden) {
            result[key] = {
                type: propDef.type,
                value: cloneValue(value),
            };
            continue;
        }

        const type = getEffectivePropertyType(node, key, propDef);
        result[key] = {
            type,
            value: normalizeValueForType(type, value),
        };
    }

    return result;
}

function getEffectivePropertyType(node, key, propDef) {
    if (node.kettleType === KETTLE_NODE_TYPES.CONSTANT && key === "value") {
        return node.properties[NODE_PROPERTIES.VALUE_TYPE] || KETTLE_TYPES.INT32;
    }

    return propDef?.type || inferPropertyType(node.properties[key]);
}

function inferPropertyType(value) {
    if (typeof value === "boolean") return KETTLE_TYPES.BOOL;
    if (typeof value === "number") return KETTLE_TYPES.FLOAT32;
    if (typeof value === "string") return KETTLE_TYPES.STRING;

    if (Array.isArray(value)) {
        if (value.length === 2) return KETTLE_TYPES.VEC2;
        if (value.length === 3) return KETTLE_TYPES.VEC3;
        if (value.length === 4) return KETTLE_TYPES.VEC4;
    }

    if (typeof value === "object" && value !== null) {
        if ("index" in value && "generation" in value) {
            return KETTLE_TYPES.OBJECT;
        }

        return KETTLE_TYPES.BLOB;
    }

    return KETTLE_TYPES.BLOB;
}

function normalizeValueForType(type, value) {
    if (type === KETTLE_TYPES.NONE) return null;

    if (type === KETTLE_TYPES.BOOL) return Boolean(value);

    if (isIntegerType(type)) return Number.parseInt(value ?? 0, 10);
    if (isFloatType(type)) return Number(value ?? 0);

    if (type === KETTLE_TYPES.STRING) return String(value ?? "");

    if (type === KETTLE_TYPES.OBJECT) {
        return {
            index: Number(value?.index ?? 0),
            generation: Number(value?.generation ?? 0),
        };
    }

    if (type === KETTLE_TYPES.VEC2) return normalizeVector(value, 2);
    if (type === KETTLE_TYPES.VEC3) return normalizeVector(value, 3);
    if (type === KETTLE_TYPES.VEC4) return normalizeVector(value, 4);

    if (type === KETTLE_TYPES.BLOB) {
        if (value === undefined || value === null || value === "") return {};
        return value;
    }

    return value;
}

function normalizeVector(value, count) {
    const out = new Array(count).fill(0.0);

    if (Array.isArray(value)) {
        for (let i = 0; i < Math.min(count, value.length); ++i) {
            out[i] = Number(value[i]);
        }
    }

    return out;
}

function defaultValueForType(type) {
    if (type === KETTLE_TYPES.NONE) return null;
    if (type === KETTLE_TYPES.BOOL) return false;
    if (isIntegerType(type)) return 0;
    if (isFloatType(type)) return 0.0;
    if (type === KETTLE_TYPES.STRING) return "";
    if (type === KETTLE_TYPES.OBJECT) return { index: 0, generation: 0 };
    if (type === KETTLE_TYPES.VEC2) return [0.0, 0.0];
    if (type === KETTLE_TYPES.VEC3) return [0.0, 0.0, 0.0];
    if (type === KETTLE_TYPES.VEC4) return [0.0, 0.0, 0.0, 0.0];
    if (type === KETTLE_TYPES.BLOB) return {};
    return null;
}

function isIntegerType(type) {
    return [
        KETTLE_TYPES.INT8,
        KETTLE_TYPES.UINT8,
        KETTLE_TYPES.INT16,
        KETTLE_TYPES.UINT16,
        KETTLE_TYPES.INT32,
        KETTLE_TYPES.UINT32,
        KETTLE_TYPES.INT64,
        KETTLE_TYPES.UINT64,
    ].includes(type);
}

function isFloatType(type) {
    return type === KETTLE_TYPES.FLOAT32 || type === KETTLE_TYPES.FLOAT64;
}

function cloneValue(value) {
    if (value === undefined) return undefined;
    return JSON.parse(JSON.stringify(value));
}

// -----------------------------------------------------------------------------
// UI
// -----------------------------------------------------------------------------

function setupToolbar() {
    const explicitButtons = document.querySelectorAll("[data-node]");

    if (explicitButtons.length > 0) {
        explicitButtons.forEach(button => {
            button.addEventListener("click", () => {
                addNode(button.dataset.node);
            });
        });
        return;
    }

    const toolbar = document.getElementById("node-buttons");
    if (!toolbar) return;

    for (const type of Object.keys(NODE_DEFS)) {
        const button = document.createElement("button");
        button.textContent = type;
        button.dataset.node = type;
        button.addEventListener("click", () => addNode(type));
        toolbar.appendChild(button);
    }
}

document.getElementById("export-btn").addEventListener("click", () => {
    const json = exportKettleJSON();
    document.getElementById("output").value = JSON.stringify(json, null, 2);
});

document.getElementById("import-input").addEventListener("change", async event => {
    const file = event.target.files[0];
    if (!file) return;

    const text = await file.text();
    const json = JSON.parse(text);
    importKettleJSON(json);
});

const importButton = document.querySelector(".import-btn");
const importInput = document.getElementById("import-input");

importButton.addEventListener("click", event => {
    event.preventDefault();
    importInput.click();
});

importInput.addEventListener("click", event => {
    event.stopPropagation();
});

// -----------------------------------------------------------------------------
// Boot
// -----------------------------------------------------------------------------

registerAllNodeTypes();
setupToolbar();
graph.start();