<template>
  <div 
    ref="containerRef" 
    class="network-animation" 
    :class="{ 'interactive': isDragging }"
  >
    <canvas ref="canvasRef"></canvas>
  </div>
</template>

<script setup>
import { ref, onMounted, onUnmounted } from 'vue'

// Canvas and container refs
const canvasRef = ref(null)
const containerRef = ref(null)

// Animation state
let ctx = null
let animationFrameId = null
let lastTime = 0
let isDarkMode = false
let canvasWidth = 0
let canvasHeight = 0
let networkInitialized = false

// Interactive state
const isDragging = ref(false)
let draggedNode = null

// Network state
let serverNode = null
let clientNodes = []

// Performance: limit DPR for high-res screens
const MAX_DPR = 2

// Pre-computed colors for performance (avoid creating objects in render loop)
const PACKET_COLORS = {
  SERVER: { r: 74, g: 222, b: 128 },   // #4ade80 Green
  CLIENT: { r: 96, g: 165, b: 250 },   // #60a5fa Blue
  MULTICAST: { r: 244, g: 114, b: 182 } // #f472b6 Pink
}

// RPC Types with colors and properties
const RPC_TYPES = {
  SERVER: {
    name: 'Server RPC',
    color: '#4ade80',
    rgb: PACKET_COLORS.SERVER,
    size: 6,
    speed: 1.0
  },
  CLIENT: {
    name: 'Client RPC', 
    color: '#60a5fa',
    rgb: PACKET_COLORS.CLIENT,
    size: 6,
    speed: 0.85
  },
  MULTICAST: {
    name: 'Multicast RPC',
    color: '#f472b6',
    rgb: PACKET_COLORS.MULTICAST,
    size: 7,
    speed: 0.7
  }
}

// Theme colors (pre-computed)
const COLORS = {
  light: {
    server: '#6366f1',
    serverGlow: 'rgba(99, 102, 241, 0.25)',
    client: '#8b5cf6',
    clientGlow: 'rgba(139, 92, 246, 0.15)',
    connection: 'rgba(99, 102, 241, 0.5)'
  },
  dark: {
    server: '#818cf8',
    serverGlow: 'rgba(129, 140, 248, 0.3)',
    client: '#a78bfa',
    clientGlow: 'rgba(167, 139, 250, 0.2)',
    connection: 'rgba(167, 139, 250, 0.45)'
  }
}

const getColors = () => isDarkMode ? COLORS.dark : COLORS.light

// Quadratic bezier point calculation (inlined for performance)
const getBezierPoint = (t, p0x, p0y, p1x, p1y, p2x, p2y) => {
  const oneMinusT = 1 - t
  const oneMinusT2 = oneMinusT * oneMinusT
  const t2 = t * t
  const twoOneMinusTt = 2 * oneMinusT * t
  return {
    x: oneMinusT2 * p0x + twoOneMinusTt * p1x + t2 * p2x,
    y: oneMinusT2 * p0y + twoOneMinusTt * p1y + t2 * p2y
  }
}

// Calculate control point for curved connection
// Always use consistent ordering so curve is same in both directions
const getControlPoint = (x1, y1, x2, y2) => {
  // Normalize: always compute from the node with smaller x (or y if x equal)
  let fromX, fromY, toX, toY
  if (x1 < x2 || (x1 === x2 && y1 < y2)) {
    fromX = x1; fromY = y1; toX = x2; toY = y2
  } else {
    fromX = x2; fromY = y2; toX = x1; toY = y1
  }
  
  const midX = (fromX + toX) * 0.5
  const midY = (fromY + toY) * 0.5
  const offsetX = (toY - fromY) * 0.15
  const offsetY = (fromX - toX) * 0.15
  return { x: midX + offsetX, y: midY + offsetY }
}

// Simple Node class (optimized)
class Node {
  constructor(x, y, isServer = false) {
    this.x = x
    this.y = y
    this.baseX = x  // Store original position as ratio
    this.baseY = y
    this.targetX = x
    this.targetY = y
    this.isServer = isServer
    this.radius = isServer ? 24 : 16
    this.pulsePhase = Math.random() * Math.PI * 2
    this.hovered = false
  }

  update(deltaTime) {
    // Smooth position interpolation when dragged
    const dx = this.targetX - this.x
    const dy = this.targetY - this.y
    if (Math.abs(dx) > 0.1 || Math.abs(dy) > 0.1) {
      this.x += dx * 0.12
      this.y += dy * 0.12
    }
    this.pulsePhase += deltaTime * 0.4
  }

  draw(ctx) {
    const colors = getColors()
    const pulse = 1 + Math.sin(this.pulsePhase) * 0.05
    const r = this.radius * pulse
    
    // Simple glow (single circle with alpha)
    ctx.beginPath()
    ctx.arc(this.x, this.y, r * 2, 0, Math.PI * 2)
    ctx.fillStyle = this.isServer ? colors.serverGlow : colors.clientGlow
    ctx.fill()
    
    // Main node
    ctx.beginPath()
    ctx.arc(this.x, this.y, r, 0, Math.PI * 2)
    ctx.fillStyle = this.isServer ? colors.server : colors.client
    ctx.fill()
    
    // Server indicator
    if (this.isServer) {
      ctx.fillStyle = 'rgba(255,255,255,0.9)'
      ctx.font = `${r * 0.7}px sans-serif`
      ctx.textAlign = 'center'
      ctx.textBaseline = 'middle'
      ctx.fillText('★', this.x, this.y)
    }
    
    // Hover ring
    if (this.hovered) {
      ctx.beginPath()
      ctx.arc(this.x, this.y, r + 4, 0, Math.PI * 2)
      ctx.strokeStyle = this.isServer ? colors.server : colors.client
      ctx.lineWidth = 2
      ctx.stroke()
    }
  }

  containsPoint(px, py) {
    const dx = this.x - px
    const dy = this.y - py
    return dx * dx + dy * dy <= this.radius * this.radius * 2.25
  }
}

// Packet pool for performance (avoids GC)
const PACKET_POOL_SIZE = 30
let packetPool = []
let activePackets = []

// Optimized Packet class - follows connection curve dynamically
class Packet {
  constructor() {
    this.fromNode = null
    this.toNode = null
    this.rpcType = null
    this.progress = 0
    this.speed = 1
    this.size = 6
    this.alive = false
    this.rotation = 0
  }

  init(fromNode, toNode, rpcType) {
    this.fromNode = fromNode
    this.toNode = toNode
    this.rpcType = rpcType
    this.progress = 0
    this.speed = rpcType.speed * (0.9 + Math.random() * 0.2)
    this.size = rpcType.size
    this.alive = true
    this.rotation = 0
    return this
  }

  update(deltaTime) {
    this.progress += deltaTime * this.speed * 0.25
    this.rotation += deltaTime * 3
    
    if (this.progress >= 1) {
      this.alive = false
    }
  }

  draw(ctx) {
    if (!this.alive) return
    
    // Get current node positions (dynamic - follows dragged nodes)
    const fromX = this.fromNode.x
    const fromY = this.fromNode.y
    const toX = this.toNode.x
    const toY = this.toNode.y
    
    // Calculate control point for this connection
    const cp = getControlPoint(fromX, fromY, toX, toY)
    
    // Get position along the bezier curve
    const t = Math.min(this.progress, 1)
    const pos = getBezierPoint(t, fromX, fromY, cp.x, cp.y, toX, toY)
    
    // Spawn/despawn scaling
    let scale = 1
    if (this.progress < 0.1) {
      scale = this.progress * 10
    } else if (this.progress > 0.9) {
      scale = (1 - this.progress) * 10
    }
    
    const effectiveSize = this.size * scale
    if (effectiveSize < 0.5) return
    
    // Draw packet (simple filled circle for performance)
    ctx.save()
    ctx.translate(pos.x, pos.y)
    ctx.rotate(this.rotation)
    
    // Glow
    ctx.beginPath()
    ctx.arc(0, 0, effectiveSize * 2, 0, Math.PI * 2)
    const { r, g, b } = this.rpcType.rgb
    ctx.fillStyle = `rgba(${r}, ${g}, ${b}, 0.3)`
    ctx.fill()
    
    // Main packet
    ctx.beginPath()
    ctx.arc(0, 0, effectiveSize, 0, Math.PI * 2)
    ctx.fillStyle = this.rpcType.color
    ctx.fill()
    
    ctx.restore()
  }
}

// Initialize packet pool
const initPacketPool = () => {
  packetPool = []
  activePackets = []
  for (let i = 0; i < PACKET_POOL_SIZE; i++) {
    packetPool.push(new Packet())
  }
}

// Get a packet from the pool
const getPacket = (fromNode, toNode, rpcType) => {
  let packet = packetPool.pop()
  if (!packet) {
    // Pool exhausted, reuse oldest active packet
    return null
  }
  packet.init(fromNode, toNode, rpcType)
  activePackets.push(packet)
  return packet
}

// Return dead packets to pool
const recyclePackets = () => {
  for (let i = activePackets.length - 1; i >= 0; i--) {
    if (!activePackets[i].alive) {
      packetPool.push(activePackets[i])
      activePackets.splice(i, 1)
    }
  }
}

// Draw connection line between nodes
const drawConnection = (ctx, from, to) => {
  const colors = getColors()
  const cp = getControlPoint(from.x, from.y, to.x, to.y)
  
  ctx.beginPath()
  ctx.moveTo(from.x, from.y)
  ctx.quadraticCurveTo(cp.x, cp.y, to.x, to.y)
  ctx.strokeStyle = colors.connection
  ctx.lineWidth = 2
  ctx.setLineDash([8, 6])
  ctx.stroke()
  ctx.setLineDash([])
}

// Generate position in edge regions (avoiding center)
const generateEdgePosition = (width, height) => {
  const margin = 50
  const regions = [
    { minX: margin, maxX: width * 0.28, minY: margin, maxY: height - margin },
    { minX: width * 0.72, maxX: width - margin, minY: margin, maxY: height - margin },
    { minX: width * 0.28, maxX: width * 0.72, minY: margin, maxY: height * 0.12 },
    { minX: width * 0.28, maxX: width * 0.72, minY: height * 0.88, maxY: height - margin }
  ]
  
  const region = regions[Math.floor(Math.random() * regions.length)]
  return {
    x: region.minX + Math.random() * (region.maxX - region.minX),
    y: region.minY + Math.random() * (region.maxY - region.minY)
  }
}

// Initialize network (only once on mount)
const initNetwork = (width, height) => {
  if (networkInitialized) return
  networkInitialized = true
  
  const nodeCount = 4 + Math.floor(Math.random() * 9) // 4-12 clients
  const minDistance = 70
  
  const positions = []
  for (let i = 0; i < nodeCount + 1; i++) {
    let attempts = 0
    let validPos = null
    
    while (!validPos && attempts < 50) {
      const pos = generateEdgePosition(width, height)
      const isValid = positions.every(existing => {
        const dx = pos.x - existing.x
        const dy = pos.y - existing.y
        return dx * dx + dy * dy >= minDistance * minDistance
      })
      
      if (isValid) validPos = pos
      attempts++
    }
    
    if (validPos) positions.push(validPos)
  }
  
  if (positions.length > 0) {
    // Pick position for server (prefer left or right side)
    const centerX = width / 2
    let bestIndex = 0
    let bestScore = Infinity
    
    positions.forEach((pos, i) => {
      const distFromCenter = Math.abs(pos.x - centerX)
      if (distFromCenter > width * 0.15) {
        const score = Math.abs(pos.y - height / 2)
        if (score < bestScore) {
          bestScore = score
          bestIndex = i
        }
      }
    })
    
    serverNode = new Node(positions[bestIndex].x, positions[bestIndex].y, true)
    clientNodes = positions
      .filter((_, i) => i !== bestIndex)
      .map(pos => new Node(pos.x, pos.y, false))
  }
  
  // Initialize packet pool
  initPacketPool()
  
  // Pre-warm: spawn initial packets at various progress points
  if (serverNode && clientNodes.length > 0) {
    const preWarmCount = Math.min(clientNodes.length, 6)
    for (let i = 0; i < preWarmCount; i++) {
      const client = clientNodes[i % clientNodes.length]
      const rpcType = i % 2 === 0 ? RPC_TYPES.SERVER : RPC_TYPES.CLIENT
      const packet = getPacket(
        rpcType === RPC_TYPES.SERVER ? client : serverNode,
        rpcType === RPC_TYPES.SERVER ? serverNode : client,
        rpcType
      )
      if (packet) {
        // Stagger progress so packets are spread along connections
        packet.progress = (i / preWarmCount) * 0.7 + 0.1
      }
    }
  }
}

// Spawn new packet
const spawnPacket = () => {
  if (!serverNode || clientNodes.length === 0) return
  
  // Weighted random: Server 40%, Client 50%, Multicast 10%
  const roll = Math.random()
  let rpcType
  if (roll < 0.4) {
    rpcType = RPC_TYPES.SERVER
  } else if (roll < 0.9) {
    rpcType = RPC_TYPES.CLIENT
  } else {
    rpcType = RPC_TYPES.MULTICAST
  }
  
  if (rpcType === RPC_TYPES.SERVER) {
    const client = clientNodes[Math.floor(Math.random() * clientNodes.length)]
    getPacket(client, serverNode, rpcType)
  } else if (rpcType === RPC_TYPES.CLIENT) {
    const client = clientNodes[Math.floor(Math.random() * clientNodes.length)]
    getPacket(serverNode, client, rpcType)
  } else {
    // Multicast - staggered to all clients
    clientNodes.forEach((client, i) => {
      setTimeout(() => {
        if (serverNode) getPacket(serverNode, client, rpcType)
      }, i * 60)
    })
  }
}

// Main animation loop
const animate = (currentTime) => {
  if (!ctx || !canvasRef.value) return
  
  const deltaTime = Math.min((currentTime - lastTime) / 1000, 0.1)
  lastTime = currentTime
  
  // Clear canvas
  ctx.clearRect(0, 0, canvasWidth, canvasHeight)
  
  // Draw connections
  clientNodes.forEach(client => {
    drawConnection(ctx, client, serverNode)
  })
  
  // Update and draw nodes
  serverNode?.update(deltaTime)
  serverNode?.draw(ctx)
  
  clientNodes.forEach(client => {
    client.update(deltaTime)
    client.draw(ctx)
  })
  
  // Update and draw packets, recycle dead ones to pool
  activePackets.forEach(packet => {
    packet.update(deltaTime)
    packet.draw(ctx)
  })
  recyclePackets()
  
  animationFrameId = requestAnimationFrame(animate)
}

// Resize handler - only resizes canvas, doesn't regenerate network
const handleResize = () => {
  if (!canvasRef.value || !containerRef.value) return
  
  const dpr = Math.min(window.devicePixelRatio || 1, MAX_DPR)
  const rect = containerRef.value.getBoundingClientRect()
  
  canvasWidth = rect.width
  canvasHeight = rect.height
  
  canvasRef.value.width = rect.width * dpr
  canvasRef.value.height = rect.height * dpr
  canvasRef.value.style.width = rect.width + 'px'
  canvasRef.value.style.height = rect.height + 'px'
  
  ctx = canvasRef.value.getContext('2d', { alpha: true })
  ctx.scale(dpr, dpr)
  
  // Initialize network only once
  if (!networkInitialized) {
    initNetwork(rect.width, rect.height)
  }
}

// Mouse/touch handlers
const getEventPos = (e) => {
  const rect = canvasRef.value.getBoundingClientRect()
  const clientX = e.touches ? e.touches[0].clientX : e.clientX
  const clientY = e.touches ? e.touches[0].clientY : e.clientY
  return { x: clientX - rect.left, y: clientY - rect.top }
}

const findNodeAtPos = (x, y) => {
  if (serverNode?.containsPoint(x, y)) return serverNode
  return clientNodes.find(node => node.containsPoint(x, y))
}

const handlePointerDown = (e) => {
  const pos = getEventPos(e)
  const node = findNodeAtPos(pos.x, pos.y)
  
  if (node) {
    draggedNode = node
    isDragging.value = true
    e.preventDefault()
  }
}

const handlePointerMove = (e) => {
  const pos = getEventPos(e)
  
  // Update hover state
  const hoveredNode = findNodeAtPos(pos.x, pos.y)
  
  if (serverNode) serverNode.hovered = false
  clientNodes.forEach(n => n.hovered = false)
  
  if (hoveredNode) {
    hoveredNode.hovered = true
    containerRef.value.style.cursor = draggedNode ? 'grabbing' : 'grab'
    // Prevent text selection when hovering over nodes
    e.preventDefault()
  } else {
    containerRef.value.style.cursor = 'default'
  }
  
  if (draggedNode) {
    draggedNode.targetX = pos.x
    draggedNode.targetY = pos.y
    e.preventDefault()
  }
}

const handlePointerUp = () => {
  draggedNode = null
  isDragging.value = false
}

// Theme observer
const observeTheme = () => {
  const observer = new MutationObserver(() => {
    isDarkMode = document.documentElement.classList.contains('dark')
  })
  observer.observe(document.documentElement, { attributes: true, attributeFilter: ['class'] })
  isDarkMode = document.documentElement.classList.contains('dark')
  return () => observer.disconnect()
}

// Lifecycle
onMounted(() => {
  handleResize()
  
  lastTime = performance.now()
  animationFrameId = requestAnimationFrame(animate)
  
  // Spawn packets at interval
  const spawnInterval = setInterval(spawnPacket, 1000)
  
  // Prevent text selection when dragging
  const handleSelectStart = (e) => {
    if (draggedNode) {
      e.preventDefault()
      return false
    }
  }
  
  // Event listeners
  window.addEventListener('resize', handleResize)
  document.addEventListener('selectstart', handleSelectStart)
  canvasRef.value?.addEventListener('mousedown', handlePointerDown)
  canvasRef.value?.addEventListener('touchstart', handlePointerDown, { passive: false })
  window.addEventListener('mousemove', handlePointerMove)
  window.addEventListener('touchmove', handlePointerMove)
  window.addEventListener('mouseup', handlePointerUp)
  window.addEventListener('touchend', handlePointerUp)
  
  const disconnectTheme = observeTheme()
  
  onUnmounted(() => {
    cancelAnimationFrame(animationFrameId)
    clearInterval(spawnInterval)
    window.removeEventListener('resize', handleResize)
    document.removeEventListener('selectstart', handleSelectStart)
    canvasRef.value?.removeEventListener('mousedown', handlePointerDown)
    canvasRef.value?.removeEventListener('touchstart', handlePointerDown)
    window.removeEventListener('mousemove', handlePointerMove)
    window.removeEventListener('touchmove', handlePointerMove)
    window.removeEventListener('mouseup', handlePointerUp)
    window.removeEventListener('touchend', handlePointerUp)
    disconnectTheme()
  })
})
</script>

<style scoped>
.network-animation {
  position: fixed;
  top: 0;
  left: 0;
  width: 100vw;
  height: 100vh;
  opacity: 0.4;
  pointer-events: auto;
  z-index: 0;
  overflow: hidden;
}

.network-animation canvas {
  width: 100%;
  height: 100%;
  display: block;
}

.network-animation.interactive {
  opacity: 0.55;
}

/* Dark mode */
.dark .network-animation {
  opacity: 0.3;
}

.dark .network-animation.interactive {
  opacity: 0.45;
}

@media (max-width: 768px) {
  .network-animation {
    opacity: 0.25;
  }
  .dark .network-animation {
    opacity: 0.2;
  }
}

@media (prefers-reduced-motion: reduce) {
  .network-animation {
    opacity: 0.15;
  }
}
</style>
