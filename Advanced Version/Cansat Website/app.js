import * as THREE from 'three'
import { OrbitControls } from 'three/addons/controls/OrbitControls.js'
import { RoomEnvironment } from 'three/addons/environments/RoomEnvironment.js'

/* ============================================================
   Telemetry simulator (ported 1:1 from the Next.js version)
   ============================================================ */
const BASE = { lat: 23.824072, lon: 90.389107 }

const round = (n, d = 2) => {
  const f = 10 ** d
  return Math.round(n * f) / f
}

function simulateTelemetry(now = Date.now()) {
  const t = now / 1000

  console.log('Simulating telemetry at t =', t.toFixed(2))

  const roll = Math.sin(t * 0.6) * 0.9
  const pitch = Math.sin(t * 0.4 + 1) * 0.7

  const accel = {
    x: -Math.sin(pitch),
    y: Math.sin(roll) * Math.cos(pitch),
    z: Math.cos(roll) * Math.cos(pitch),
  }

  const gyro = {
    x: round(Math.cos(t * 0.6) * 34 + (Math.random() - 0.5) * 2),
    y: round(Math.cos(t * 0.4 + 1) * 25 + (Math.random() - 0.5) * 2),
    z: round(28 + Math.sin(t * 0.2) * 10 + (Math.random() - 0.5) * 2),
  }

  const cycle = (t % 120) / 120
  const alt = 120 - Math.abs(Math.sin(cycle * Math.PI)) * 95

  const lat = BASE.lat + Math.sin(t * 0.05) * 0.0016
  const lon = BASE.lon + Math.cos(t * 0.05) * 0.0016

  return {
    packet_id: Math.floor(now / 1000) % 100000,

    timestamp: now,

    mpu: {
      accel: {
        x: round(accel.x, 3),
        y: round(accel.y, 3),
        z: round(accel.z, 3),
      },

      gyro,

      temp_c: round(44 + Math.sin(t * 0.1) * 3),
    },

    bme688: {
      temp_c: round(30 + Math.sin(t * 0.08) * 2.5),
      humidity: round(68 + Math.sin(t * 0.12 + 2) * 6),
      pressure_hpa: round(1007 + Math.sin(t * 0.05) * 4),
      gas_kohm: round(105 + Math.sin(t * 0.15) * 12),
    },

    gps: {
      lat: round(lat, 6),
      lon: round(lon, 6),
      alt_m: round(alt),
      satellites:
        5 +
        (Math.sin(t * 0.1) > 0.4 ? 1 : 0) +
        (Math.sin(t * 0.3) > 0.6 ? 1 : 0),
      fix: true,
    },

    lora: {
      rssi: Math.round(-92 + Math.sin(t * 0.2) * 12),
      snr: round(9 + Math.sin(t * 0.25) * 3.5),
    },
  }
}

/* ============================================================
   Helpers
   ============================================================ */

const toDeg = (rad) => (rad * 180) / Math.PI

const $ = (id) => document.getElementById(id)

function signalQuality(rssi) {
  if (rssi >= -80) {
    return {
      label: 'Excellent',
      color: 'var(--accent)',
      pct: 1,
    }
  }

  if (rssi >= -95) {
    return {
      label: 'Good',
      color: 'var(--primary)',
      pct: 0.7,
    }
  }

  if (rssi >= -110) {
    return {
      label: 'Fair',
      color: 'var(--chart-3)',
      pct: 0.45,
    }
  }

  return {
    label: 'Weak',
    color: 'var(--destructive)',
    pct: 0.2,
  }
}

function deriveOrientation(mpu) {
  const { x, y, z } = mpu.accel

  const roll = Math.atan2(y, z)

  const pitch = Math.atan2(
    -x,
    Math.sqrt(y * y + z * z)
  )

  return {
    pitch,
    roll,
    yawRate: mpu.gyro.z,
  }
}

/* ============================================================
   3D attitude viewer (Three.js — mirrors the R3F scene)
   ============================================================ */

let orientation = {
  pitch: 0,
  roll: 0,
  yawRate: 0,
}

function initThree() {
  const mount = $('three-mount')

  const width = mount.clientWidth || 600
  const height = mount.clientHeight || 420

  const renderer = new THREE.WebGLRenderer({
    antialias: true,
    powerPreference: 'high-performance',
  })

  renderer.setPixelRatio(
    Math.min(window.devicePixelRatio, 2)
  )

  renderer.setSize(width, height)

  renderer.shadowMap.enabled = true

  mount.appendChild(renderer.domElement)

  const scene = new THREE.Scene()

  scene.background = new THREE.Color('#0d1220')

  scene.fog = new THREE.Fog(
    '#0d1220',
    9,
    20
  )

  // Metallic reflections without an external HDRI.
  const pmrem = new THREE.PMREMGenerator(renderer)

  scene.environment =
    pmrem.fromScene(
      new RoomEnvironment(),
      0.04
    ).texture

  const camera = new THREE.PerspectiveCamera(
    42,
    width / height,
    0.1,
    100
  )

  camera.position.set(
    4,
    2.5,
    5
  )

  scene.add(
    new THREE.AmbientLight(
      0xffffff,
      0.5
    )
  )

  scene.add(
    new THREE.HemisphereLight(
      0xcbd5e1,
      0x0f172a,
      0.6
    )
  )

  const dir =
    new THREE.DirectionalLight(
      0xffffff,
      1.6
    )

  dir.position.set(
    5,
    8,
    5
  )

  dir.castShadow = true

  scene.add(dir)

  const dir2 =
    new THREE.DirectionalLight(
      0x22d3ee,
      0.5
    )

  dir2.position.set(
    -5,
    2,
    -3
  )

  scene.add(dir2)

  const point =
    new THREE.PointLight(
      0x38bdf8,
      20
    )

  point.position.set(
    0,
    3,
    4
  )

  scene.add(point)

  // ----- CanSat body -----

  const group =
    new THREE.Group()

  const std = (opts) =>
    new THREE.MeshStandardMaterial(opts)

  const body =
    new THREE.Mesh(
      new THREE.CylinderGeometry(
        0.9,
        0.9,
        2.2,
        48
      ),
      std({
        color: '#c7cdd6',
        metalness: 0.85,
        roughness: 0.28,
      })
    )

  body.castShadow = true
  body.receiveShadow = true

  group.add(body)

  const bandMat = () =>
    std({
      color: '#22d3ee',
      metalness: 0.6,
      roughness: 0.3,
      emissive: new THREE.Color('#0891b2'),
      emissiveIntensity: 0.4,
    })

  const bandTop =
    new THREE.Mesh(
      new THREE.CylinderGeometry(
        0.92,
        0.92,
        0.18,
        48
      ),
      bandMat()
    )

  bandTop.position.y = 0.7

  group.add(bandTop)

  const bandBottom =
    new THREE.Mesh(
      new THREE.CylinderGeometry(
        0.92,
        0.92,
        0.18,
        48
      ),
      bandMat()
    )

  bandBottom.position.y = -0.7

  group.add(bandBottom)

  const capTop =
    new THREE.Mesh(
      new THREE.CylinderGeometry(
        0.86,
        0.9,
        0.12,
        48
      ),
      std({
        color: '#8b94a3',
        metalness: 0.9,
        roughness: 0.35,
      })
    )

  capTop.position.y = 1.12

  group.add(capTop)

  const capBottom =
    new THREE.Mesh(
      new THREE.CylinderGeometry(
        0.9,
        0.86,
        0.12,
        48
      ),
      std({
        color: '#8b94a3',
        metalness: 0.9,
        roughness: 0.35,
      })
    )

  capBottom.position.y = -1.12

  group.add(capBottom)

  const mast =
    new THREE.Mesh(
      new THREE.CylinderGeometry(
        0.04,
        0.04,
        1.1,
        12
      ),
      std({
        color: '#e2e8f0',
        metalness: 0.7,
        roughness: 0.4,
      })
    )

  mast.position.set(
    0.45,
    1.7,
    0
  )

  group.add(mast)

  const mastTip =
    new THREE.Mesh(
      new THREE.SphereGeometry(
        0.09,
        16,
        16
      ),
      std({
        color: '#34d399',
        emissive: new THREE.Color('#10b981'),
        emissiveIntensity: 0.9,
      })
    )

  mastTip.position.set(
    0.45,
    2.3,
    0
  )

  group.add(mastTip)

  const portOuter =
    new THREE.Mesh(
      new THREE.CylinderGeometry(
        0.22,
        0.22,
        0.1,
        24
      ),
      std({
        color: '#0f172a',
        metalness: 0.3,
        roughness: 0.2,
      })
    )

  portOuter.position.set(
    0,
    0,
    0.92
  )

  portOuter.rotation.x =
    Math.PI / 2

  group.add(portOuter)

  const portInner =
    new THREE.Mesh(
      new THREE.CylinderGeometry(
        0.12,
        0.12,
        0.05,
        24
      ),
      std({
        color: '#38bdf8',
        emissive: new THREE.Color('#0ea5e9'),
        emissiveIntensity: 0.7,
      })
    )

  portInner.position.set(
    0,
    0,
    0.98
  )

  portInner.rotation.x =
    Math.PI / 2

  group.add(portInner)

  scene.add(group)

  // ----- Grid -----

  const grid =
    new THREE.GridHelper(
      30,
      50,
      0x334155,
      0x1e293b
    )

  grid.position.y = -2

  scene.add(grid)

  const controls =
    new OrbitControls(
      camera,
      renderer.domElement
    )

  controls.enablePan = false

  controls.minDistance = 4

  controls.maxDistance = 11

  controls.enableDamping = true

  let yaw = 0

  const clock =
    new THREE.Clock()

  function animate() {
    requestAnimationFrame(animate)

    const delta =
      clock.getDelta()

    group.rotation.x +=
      (
        orientation.pitch -
        group.rotation.x
      ) *
      Math.min(
        1,
        delta * 6
      )

    group.rotation.z +=
      (
        orientation.roll -
        group.rotation.z
      ) *
      Math.min(
        1,
        delta * 6
      )

    yaw +=
      (
        (orientation.yawRate * Math.PI) /
        180
      ) *
      delta

    group.rotation.y = yaw

    controls.update()

    renderer.render(
      scene,
      camera
    )
  }

  animate()

  const resize = () => {
    const w =
      mount.clientWidth

    const h =
      mount.clientHeight

    if (!w || !h) return

    camera.aspect =
      w / h

    camera.updateProjectionMatrix()

    renderer.setSize(
      w,
      h
    )
  }

  window.addEventListener(
    'resize',
    resize
  )

  // Handle the responsive height change
  // (360 -> 420) once layout settles.
  setTimeout(
    resize,
    300
  )
}

/* ============================================================
   Leaflet ground track
   ============================================================ */

let map
let marker
let trailLine

let trail = []

let lastKey = ''

function initMap(lat, lon) {
  const satIcon =
    L.divIcon({
      className: '',
      html:
        '<div class="cansat-marker">' +
        '<span class="cansat-marker__pulse"></span>' +
        '<span class="cansat-marker__dot"></span>' +
        '</div>',
      iconSize: [
        22,
        22,
      ],
      iconAnchor: [
        11,
        11,
      ],
    })

  map =
    L.map('map', {
      scrollWheelZoom: true,
      zoomControl: true,
    }).setView(
      [lat, lon],
      16
    )

  L.tileLayer(
    'https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png',
    {
      attribution:
        '&copy; <a href="https://www.openstreetmap.org/copyright">OpenStreetMap</a> contributors',
    }
  ).addTo(map)

  trail = [
    [lat, lon]
  ]

  trailLine =
    L.polyline(
      trail,
      {
        color: '#22d3ee',
        weight: 3,
        opacity: 0.8,
      }
    ).addTo(map)

  marker =
    L.marker(
      [lat, lon],
      {
        icon: satIcon,
      }
    ).addTo(map)
}

function updateMap(lat, lon) {
  if (!map) {
    initMap(
      lat,
      lon
    )

    return
  }

  const key =
    `${lat},${lon}`

  if (key !== lastKey) {
    lastKey = key

    trail.push([
      lat,
      lon
    ])

    if (trail.length > 120) {
      trail =
        trail.slice(-120)
    }

    trailLine.setLatLngs(
      trail
    )

    marker.setLatLng([
      lat,
      lon
    ])

    map.setView(
      [lat, lon],
      map.getZoom(),
      {
        animate: true,
      }
    )
  }
}

/* ============================================================
   DOM updates on each telemetry packet
   ============================================================ */

function setText(id, value) {
  const el = $(id)

  if (el) {
    el.textContent = value
  }
}

function updateDashboard(t) {

  // Status

  const pill =
    $('status-pill')

  pill.classList.add(
    'is-live'
  )

  setText(
    'status-label',
    'Live Link'
  )

  setText(
    'packet',
    `#${t.packet_id}`
  )

  setText(
    'updated',
    new Date(
      t.timestamp
    ).toLocaleTimeString()
  )

  // Orientation feeding the 3D scene

  orientation =
    deriveOrientation(
      t.mpu
    )

  const pitchDeg =
    toDeg(
      Math.atan2(
        -t.mpu.accel.x,
        Math.sqrt(
          t.mpu.accel.y ** 2 +
          t.mpu.accel.z ** 2
        )
      )
    )

  const rollDeg =
    toDeg(
      Math.atan2(
        t.mpu.accel.y,
        t.mpu.accel.z
      )
    )

  setText(
    'roll',
    `${rollDeg.toFixed(1)}°`
  )

  setText(
    'pitch',
    `${pitchDeg.toFixed(1)}°`
  )

  setText(
    'yaw',
    `${t.mpu.gyro.z.toFixed(1)}°/s`
  )

  // Accel / gyro rows

  setText(
    'ax',
    t.mpu.accel.x.toFixed(2)
  )

  setText(
    'ay',
    t.mpu.accel.y.toFixed(2)
  )

  setText(
    'az',
    t.mpu.accel.z.toFixed(2)
  )

  setText(
    'gx',
    t.mpu.gyro.x.toFixed(2)
  )

  setText(
    'gy',
    t.mpu.gyro.y.toFixed(2)
  )

  setText(
    'gz',
    t.mpu.gyro.z.toFixed(2)
  )

  // GPS

  setText(
    'lat',
    t.gps.lat.toFixed(6)
  )

  setText(
    'lon',
    t.gps.lon.toFixed(6)
  )

  setText(
    'alt',
    t.gps.alt_m.toFixed(1)
  )

  setText(
    'sats',
    `${t.gps.satellites}`
  )

  const fixHint =
    $('fix-hint')

  const satIcon =
    $('sat-icon')

  fixHint.textContent =
    t.gps.fix
      ? 'Fix acquired'
      : 'No fix'

  fixHint.style.color =
    t.gps.fix
      ? 'var(--accent)'
      : 'var(--destructive)'

  satIcon.style.color =
    t.gps.fix
      ? 'var(--accent)'
      : 'var(--destructive)'

  updateMap(
    t.gps.lat,
    t.gps.lon
  )

  // Environment

  setText(
    'bme-temp',
    t.bme688.temp_c.toFixed(1)
  )

  $('bme-temp-bar').style.width =
    `${Math.min(
      1,
      t.bme688.temp_c / 60
    ) * 100}%`

  setText(
    'humidity',
    t.bme688.humidity.toFixed(1)
  )

  $('humidity-bar').style.width =
    `${Math.min(
      1,
      t.bme688.humidity / 100
    ) * 100}%`

  setText(
    'pressure',
    t.bme688.pressure_hpa.toFixed(1)
  )

  setText(
    'gas',
    t.bme688.gas_kohm.toFixed(1)
  )

  $('gas-bar').style.width =
    `${Math.min(
      1,
      t.bme688.gas_kohm / 200
    ) * 100}%`

  // LoRa

  const sig =
    signalQuality(
      t.lora.rssi
    )

  setText(
    'rssi',
    `${t.lora.rssi}`
  )

  const rssiBar =
    $('rssi-bar')

  rssiBar.style.width =
    `${sig.pct * 100}%`

  rssiBar.style.color =
    sig.color

  $('rssi-icon').style.color =
    sig.color

  const rssiHint =
    $('rssi-hint')

  rssiHint.textContent =
    `${sig.label} link`

  setText(
    'snr',
    t.lora.snr.toFixed(2)
  )

  // Footer

  setText(
    'mpu-temp',
    `MPU temp: ${t.mpu.temp_c.toFixed(1)} °C`
  )
}

/* ============================================================
   Web Serial
   ============================================================ */

let serialPort = null
let serialReader = null
let serialBuffer = ''

async function connectArduino() {
  try {

    serialPort =
      await navigator.serial.requestPort()

    await serialPort.open({
      baudRate: 115200
    })

    console.log(
      'Arduino connected'
    )

    readArduinoData()

  } catch (error) {

    console.error(
      'Serial connection failed:',
      error
    )
  }
}

async function readArduinoData() {

  const decoder =
    new TextDecoderStream()

  serialPort.readable.pipeTo(
    decoder.writable
  )

  serialReader =
    decoder.readable.getReader()

  try {

    while (true) {

      const {
        value,
        done
      } =
        await serialReader.read()

      if (done) break

      if (value) {

        serialBuffer += value

        const lines =
          serialBuffer.split('\n')

        serialBuffer =
          lines.pop()

        for (
          const line of lines
        ) {

          const cleanLine =
            line.trim()

          if (!cleanLine) continue

          try {

            const telemetry =
              JSON.parse(
                cleanLine
              )

            console.log(
              'Received:',
              telemetry
            )

            // Add browser timestamp

            telemetry.timestamp =
              Date.now()

            // Update dashboard

            updateDashboard(
              telemetry
            )

          } catch (error) {

            console.warn(
              'Not JSON:',
              cleanLine
            )
          }
        }
      }
    }

  } catch (error) {

    console.error(
      'Serial reading error:',
      error
    )
  }
}

/* ============================================================
   Boot
   ============================================================ */

lucide.createIcons()

initThree()

const connectButton =
  document.querySelector(
    '.connect-arduino'
  )

if (connectButton) {

  connectButton.addEventListener(
    'click',
    connectArduino
  )
}