import { useRef, useEffect, useCallback } from 'react'
import { useFrame, useThree } from '@react-three/fiber'
import * as THREE from 'three'
import { useGameState } from '../hooks/useGameState'

const MOVE_SPEED = 12
const MOUSE_SENSITIVITY = 0.002
const PLAYER_HEIGHT = 1.7
const GRAVITY = -20
const JUMP_FORCE = 8

export function Player() {
  const { camera, gl } = useThree()
  const velocity = useRef(new THREE.Vector3())
  const direction = useRef(new THREE.Vector3())
  const euler = useRef(new THREE.Euler(0, 0, 0, 'YXZ'))
  const isLocked = useRef(false)
  const isGrounded = useRef(true)
  const keys = useRef<Set<string>>(new Set())

  const { isPlaying, bluePortal, orangePortal } = useGameState()

  const handleKeyDown = useCallback((e: KeyboardEvent) => {
    keys.current.add(e.code)
    if (e.code === 'Space' && isGrounded.current) {
      velocity.current.y = JUMP_FORCE
      isGrounded.current = false
    }
  }, [])

  const handleKeyUp = useCallback((e: KeyboardEvent) => {
    keys.current.delete(e.code)
  }, [])

  const handleMouseMove = useCallback((e: MouseEvent) => {
    if (!isLocked.current) return
    euler.current.setFromQuaternion(camera.quaternion)
    euler.current.y -= e.movementX * MOUSE_SENSITIVITY
    euler.current.x -= e.movementY * MOUSE_SENSITIVITY
    euler.current.x = Math.max(-Math.PI / 2 + 0.01, Math.min(Math.PI / 2 - 0.01, euler.current.x))
    camera.quaternion.setFromEuler(euler.current)
  }, [camera])

  const handlePointerLockChange = useCallback(() => {
    isLocked.current = document.pointerLockElement === gl.domElement
  }, [gl])

  useEffect(() => {
    document.addEventListener('keydown', handleKeyDown)
    document.addEventListener('keyup', handleKeyUp)
    document.addEventListener('mousemove', handleMouseMove)
    document.addEventListener('pointerlockchange', handlePointerLockChange)

    return () => {
      document.removeEventListener('keydown', handleKeyDown)
      document.removeEventListener('keyup', handleKeyUp)
      document.removeEventListener('mousemove', handleMouseMove)
      document.removeEventListener('pointerlockchange', handlePointerLockChange)
    }
  }, [handleKeyDown, handleKeyUp, handleMouseMove, handlePointerLockChange])

  const teleportCooldown = useRef(0)

  const checkPortalTeleport = useCallback((position: THREE.Vector3) => {
    const now = performance.now() / 1000
    if (now - teleportCooldown.current < 1) return // 1 second cooldown

    const portals = [bluePortal, orangePortal].filter(Boolean)
    if (portals.length < 2) return

    const blue = bluePortal!
    const orange = orangePortal!

    // Check if player is near blue portal -> teleport to orange
    const distToBlue = position.distanceTo(new THREE.Vector3(...blue.position))
    if (distToBlue < 2) {
      const normal = new THREE.Vector3(...orange.normal)
      const targetPos = new THREE.Vector3(...orange.position).add(normal.multiplyScalar(2))
      camera.position.copy(targetPos)
      // Preserve horizontal velocity, reset vertical
      velocity.current.y = Math.max(velocity.current.y, 2)
      teleportCooldown.current = now
      return
    }

    // Check if player is near orange portal -> teleport to blue
    const distToOrange = position.distanceTo(new THREE.Vector3(...orange.position))
    if (distToOrange < 2) {
      const normal = new THREE.Vector3(...blue.normal)
      const targetPos = new THREE.Vector3(...blue.position).add(normal.multiplyScalar(2))
      camera.position.copy(targetPos)
      velocity.current.y = Math.max(velocity.current.y, 2)
      teleportCooldown.current = now
      return
    }
  }, [bluePortal, orangePortal, camera])

  const checkBounds = useCallback((pos: THREE.Vector3) => {
    const BOUND = 28
    const posClamped = pos.clone()
    posClamped.x = Math.max(-BOUND, Math.min(BOUND, posClamped.x))
    posClamped.z = Math.max(-BOUND, Math.min(BOUND, posClamped.z))
    return posClamped
  }, [])

  useFrame((_, delta) => {
    if (!isPlaying) return
    const dt = Math.min(delta, 0.1)

    // Movement direction
    direction.current.set(0, 0, 0)
    const forward = new THREE.Vector3()
    camera.getWorldDirection(forward)
    forward.y = 0
    forward.normalize()
    const right = new THREE.Vector3().crossVectors(forward, new THREE.Vector3(0, 1, 0)).normalize()

    if (keys.current.has('KeyW')) direction.current.add(forward)
    if (keys.current.has('KeyS')) direction.current.sub(forward)
    if (keys.current.has('KeyD')) direction.current.add(right)
    if (keys.current.has('KeyA')) direction.current.sub(right)

    if (direction.current.length() > 0) {
      direction.current.normalize()
    }

    // Apply movement
    velocity.current.x = direction.current.x * MOVE_SPEED
    velocity.current.z = direction.current.z * MOVE_SPEED

    // Gravity
    velocity.current.y += GRAVITY * dt

    // Update position
    const newPos = camera.position.clone()
    newPos.x += velocity.current.x * dt
    newPos.y += velocity.current.y * dt
    newPos.z += velocity.current.z * dt

    // Ground collision
    if (newPos.y < PLAYER_HEIGHT) {
      newPos.y = PLAYER_HEIGHT
      velocity.current.y = 0
      isGrounded.current = true
    }

    // Check platform collisions (simple box collisions)
    const platforms = [
      { min: new THREE.Vector3(-5, 3, -5), max: new THREE.Vector3(5, 3.5, 5) },
      { min: new THREE.Vector3(10, 2, 10), max: new THREE.Vector3(16, 2.5, 16) },
      { min: new THREE.Vector3(-16, 2, -16), max: new THREE.Vector3(-10, 2.5, -10) },
      { min: new THREE.Vector3(10, 2, -16), max: new THREE.Vector3(16, 2.5, -10) },
      { min: new THREE.Vector3(-16, 2, 10), max: new THREE.Vector3(-10, 2.5, 16) },
    ]

    for (const plat of platforms) {
      if (
        newPos.x > plat.min.x && newPos.x < plat.max.x &&
        newPos.z > plat.min.z && newPos.z < plat.max.z
      ) {
        if (newPos.y - PLAYER_HEIGHT < plat.max.y && newPos.y > plat.min.y) {
          if (velocity.current.y <= 0) {
            newPos.y = plat.max.y + PLAYER_HEIGHT
            velocity.current.y = 0
            isGrounded.current = true
          }
        }
      }
    }

    // Wall collisions
    const walls = [
      { min: new THREE.Vector3(-3, 0, -1), max: new THREE.Vector3(3, 4, 1) },
      { min: new THREE.Vector3(-1, 0, -3), max: new THREE.Vector3(1, 4, 3) },
    ]

    for (const wall of walls) {
      if (
        newPos.x > wall.min.x - 0.5 && newPos.x < wall.max.x + 0.5 &&
        newPos.z > wall.min.z - 0.5 && newPos.z < wall.max.z + 0.5 &&
        newPos.y > wall.min.y && newPos.y < wall.max.y + PLAYER_HEIGHT
      ) {
        // Push player out
        const center = new THREE.Vector3().addVectors(wall.min, wall.max).multiplyScalar(0.5)
        const diff = new THREE.Vector3().subVectors(newPos, center)
        if (Math.abs(diff.x) > Math.abs(diff.z)) {
          newPos.x = diff.x > 0 ? wall.max.x + 0.5 : wall.min.x - 0.5
        } else {
          newPos.z = diff.z > 0 ? wall.max.z + 0.5 : wall.min.z - 0.5
        }
      }
    }

    // Bounds
    const bounded = checkBounds(newPos)
    camera.position.copy(bounded)

    // Portal teleportation
    checkPortalTeleport(camera.position)
  })

  return null
}
