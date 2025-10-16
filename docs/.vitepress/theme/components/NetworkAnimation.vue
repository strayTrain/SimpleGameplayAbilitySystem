<template>
  <div class="network-animation">
    <svg viewBox="0 0 1600 900" xmlns="http://www.w3.org/2000/svg" preserveAspectRatio="xMidYMid slice">
      <!-- Connection Lines -->
      <path
        v-for="(path, index) in connectionPaths"
        :key="'line-' + index"
        :d="path"
        class="connection-line"
      />

      <!-- Server Node -->
      <circle
        :cx="serverNode.x"
        :cy="serverNode.y"
        r="50"
        class="node server-node"
      />

      <!-- Client Nodes -->
      <circle
        v-for="(node, index) in clientNodes"
        :key="'client-' + index"
        :cx="node.x"
        :cy="node.y"
        r="35"
        class="node client-node"
      />

      <!-- Animated Packets -->
      <template v-for="(packet, index) in packets" :key="'packet-' + index">
        <rect
          :width="packet.width"
          :height="packet.height"
          :x="-packet.width / 2"
          :y="-packet.height / 2"
          :class="'packet ' + (packet.toServer ? 'packet-to-server' : 'packet-from-server')">
          <animateMotion
            :dur="packet.duration + 's'"
            repeatCount="indefinite"
            :begin="packet.delay + 's'"
            :path="packet.path"
          />
          <animateTransform
            attributeName="transform"
            attributeType="XML"
            type="scale"
            values="0 0;1.3 1.3;1 1;1 1;0 0"
            keyTimes="0;0.05;0.1;0.9;1"
            :dur="packet.duration + 's'"
            repeatCount="indefinite"
            :begin="packet.delay + 's'"
            additive="sum"
          />
        </rect>
      </template>
    </svg>
  </div>
</template>

<script setup>
import { ref, onMounted } from 'vue'

const serverNode = ref({ x: 800, y: 450 })
const clientNodes = ref([])
const connectionPaths = ref([])
const packets = ref([])

function generateNetwork() {
  // Generate random number of nodes (3-12)
  const nodeCount = Math.floor(Math.random() * 10) + 3
  const allNodes = []
  const minDistance = 35 * 3 // 1.5x the client node radius (35) * 2 = diameter * 1.5

  // Define the center exclusion zone (where content typically lives)
  const centerExclusionZone = {
    minX: 400,
    maxX: 1200,
    minY: 250,
    maxY: 650
  }

  // Helper function to check if a point is in the center exclusion zone
  const isInCenterZone = (x, y) => {
    return x >= centerExclusionZone.minX &&
           x <= centerExclusionZone.maxX &&
           y >= centerExclusionZone.minY &&
           y <= centerExclusionZone.maxY
  }

  // Helper function to generate a position in edge regions
  const generateEdgePosition = () => {
    // Define regions: left, right, top, bottom
    const regions = [
      { name: 'left', minX: 50, maxX: 350, minY: 50, maxY: 850 },
      { name: 'right', minX: 1250, maxX: 1550, minY: 50, maxY: 850 },
      { name: 'top', minX: 350, maxX: 1250, minY: 50, maxY: 200 },
      { name: 'bottom', minX: 350, maxX: 1250, minY: 700, maxY: 850 }
    ]

    const region = regions[Math.floor(Math.random() * regions.length)]
    return {
      x: Math.random() * (region.maxX - region.minX) + region.minX,
      y: Math.random() * (region.maxY - region.minY) + region.minY
    }
  }

  // Generate all node positions with minimum distance constraint
  for (let i = 0; i < nodeCount; i++) {
    let attempts = 0
    let validPosition = false
    let newNode

    while (!validPosition && attempts < 100) {
      newNode = generateEdgePosition()

      // Double-check it's not in center zone (shouldn't be, but just in case)
      if (isInCenterZone(newNode.x, newNode.y)) {
        attempts++
        continue
      }

      // Check if this position is far enough from all existing nodes
      validPosition = allNodes.every(existingNode => {
        const dx = newNode.x - existingNode.x
        const dy = newNode.y - existingNode.y
        const distance = Math.sqrt(dx * dx + dy * dy)
        return distance >= minDistance
      })

      attempts++
    }

    if (validPosition) {
      allNodes.push(newNode)
    }
  }

  // Pick random node as server
  const serverIndex = Math.floor(Math.random() * allNodes.length)
  serverNode.value = allNodes[serverIndex]

  // Rest are clients
  clientNodes.value = allNodes.filter((_, i) => i !== serverIndex)

  // Generate connection paths and packets
  connectionPaths.value = []
  packets.value = []

  clientNodes.value.forEach((client, index) => {
    const server = serverNode.value

    // Calculate connection line (curved path)
    const midX = (client.x + server.x) / 2
    const midY = (client.y + server.y) / 2
    const offsetX = (server.y - client.y) * 0.15 // Perpendicular offset for curve
    const offsetY = (client.x - server.x) * 0.15

    // Client to server path
    const clientToServer = `M ${client.x} ${client.y} Q ${midX + offsetX} ${midY + offsetY} ${server.x} ${server.y}`
    connectionPaths.value.push(clientToServer)

    // Server to client path (reverse)
    const serverToClient = `M ${server.x} ${server.y} Q ${midX + offsetX} ${midY + offsetY} ${client.x} ${client.y}`

    // Generate packets for this connection
    const baseDuration = 3 + Math.random() * 2 // 3-5 seconds

    // Helper function to generate random packet size
    const randomPacketSize = () => {
      const sizes = [
        { width: 12, height: 12 },  // Small square
        { width: 16, height: 12 },  // Wide rectangle
        { width: 12, height: 16 },  // Tall rectangle
        { width: 18, height: 10 },  // Very wide
        { width: 10, height: 18 },  // Very tall
        { width: 14, height: 14 },  // Medium square
      ]
      return sizes[Math.floor(Math.random() * sizes.length)]
    }

    // Client to server packet
    packets.value.push({
      path: clientToServer,
      duration: baseDuration,
      delay: Math.random() * 4,
      toServer: true,
      ...randomPacketSize()
    })

    // Server to client packet
    packets.value.push({
      path: serverToClient,
      duration: baseDuration,
      delay: Math.random() * 4,
      toServer: false,
      ...randomPacketSize()
    })

    // Sometimes add extra packets
    if (Math.random() > 0.5) {
      packets.value.push({
        path: clientToServer,
        duration: baseDuration,
        delay: Math.random() * 4,
        toServer: true,
        ...randomPacketSize()
      })
    }
  })
}

onMounted(() => {
  generateNetwork()
})
</script>

<style scoped>
.network-animation {
  position: fixed;
  top: 0;
  left: 0;
  width: 100vw;
  height: 100vh;
  opacity: 0.12;
  pointer-events: none;
  z-index: 0;
  overflow: hidden;
}

.network-animation svg {
  width: 100%;
  height: 100%;
  display: block;
}

.node {
  fill: var(--vp-c-brand);
  stroke: var(--vp-c-brand-light);
  stroke-width: 2;
  opacity: 0.8;
}

.server-node {
  fill: var(--vp-c-brand-dark);
  stroke: var(--vp-c-brand);
  stroke-width: 3;
}

.client-node {
  opacity: 1;
}

.connection-line {
  fill: none;
  stroke: var(--vp-c-brand-dark);
  stroke-width: 2;
  stroke-dasharray: 5, 5;
  opacity: 0.7;
}

.dark .connection-line {
  stroke: var(--vp-c-brand-lighter);
  opacity: 0.4;
}

.packet {
  opacity: 0.9;
}

.packet-to-server {
  fill: var(--vp-c-brand-lighter);
}

.packet-from-server {
  fill: var(--vp-c-brand);
}

/* Dark mode adjustments */
.dark .node-label {
  fill: var(--vp-c-text-2);
}

@media (max-width: 768px) {
  .network-animation {
    opacity: 0.06;
  }
}
</style>
