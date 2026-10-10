import { useRef, useCallback, useEffect } from 'react'
import { useFrame, useThree } from '@react-three/fiber'
import * as THREE from 'three'
import { useGameState } from '../hooks/useGameState'
import { soundManager } from '../utils/sounds'

const SHOOT_COOLDOWN = 0.12
const DAMAGE = 34
const RAY_LENGTH = 100

export function Weapon() {
  const { camera, scene } = useThree()
  const {
    ammo, useAmmo, reload, isPlaying, enemies, damageEnemy,
    addScore, addKill, setBluePortal, setOrangePortal
  } = useGameState()

  const lastShot = useRef(0)
  const isReloading = useRef(false)
  const weaponMode = useRef<'gun' | 'blue' | 'orange'>('gun')
  const hitMarkerTimer = useRef(0)

  const shoot = useCallback(() => {
    if (ammo <= 0 || isReloading.current) return

    const now = performance.now() / 1000
    if (now - lastShot.current < SHOOT_COOLDOWN) return
    lastShot.current = now

    useAmmo(1)
    soundManager.playShoot()

    // Raycasting for enemies
    const raycaster = new THREE.Raycaster()
    raycaster.setFromCamera(new THREE.Vector2(0, 0), camera)
    raycaster.far = RAY_LENGTH

    // Find enemies that intersect with the ray
    let closestHit: { enemyId: string; distance: number } | null = null

    enemies.forEach(enemy => {
      if (!enemy.isAlive) return
      const enemyPos = new THREE.Vector3(...enemy.position)
      // Simple sphere intersection test
      const toEnemy = enemyPos.clone().sub(camera.position)
      const projLength = toEnemy.dot(raycaster.ray.direction)
      if (projLength < 0 || projLength > RAY_LENGTH) return

      const closestPoint = camera.position.clone().add(
        raycaster.ray.direction.clone().multiplyScalar(projLength)
      )
      const dist = closestPoint.distanceTo(enemyPos)

      if (dist < 0.8 && (!closestHit || projLength < closestHit.distance)) {
        closestHit = { enemyId: enemy.id, distance: projLength }
      }
    })

    if (closestHit !== null) {
      const hitData = closestHit as { enemyId: string; distance: number }
      const enemy = enemies.find(e => e.id === hitData.enemyId)
      if (enemy) {
        damageEnemy(hitData.enemyId, DAMAGE)
        addScore(10)
        hitMarkerTimer.current = 0.3
        soundManager.playHit()

        if (enemy.health <= DAMAGE) {
          addKill()
          addScore(90)
          soundManager.playKill()
        }
      }
    }
  }, [ammo, camera, enemies, useAmmo, damageEnemy, addScore, addKill])

  const placePortal = useCallback((type: 'blue' | 'orange') => {
    const raycaster = new THREE.Raycaster()
    raycaster.setFromCamera(new THREE.Vector2(0, 0), camera)
    raycaster.far = 50

    // Get all meshes in scene
    const meshes: THREE.Mesh[] = []
    scene.traverse((obj) => {
      if (obj instanceof THREE.Mesh && !obj.userData.isEnemy && !obj.userData.isPortal) {
        meshes.push(obj)
      }
    })

    const intersects = raycaster.intersectObjects(meshes, false)
    const validHit = intersects.find(hit => hit.face)

    if (validHit && validHit.face) {
      // Transform normal to world space
      const normalMatrix = new THREE.Matrix3().getNormalMatrix(validHit.object.matrixWorld)
      const worldNormal = validHit.face.normal.clone().applyMatrix3(normalMatrix).normalize()

      const portal = {
        position: validHit.point.toArray() as [number, number, number],
        normal: worldNormal.toArray() as [number, number, number],
        id: `${type}-${Date.now()}`
      }

      if (type === 'blue') {
        setBluePortal(portal)
      } else {
        setOrangePortal(portal)
      }
      soundManager.playPortalPlace()
    }
  }, [camera, scene, setBluePortal, setOrangePortal])

  const handleMouseDown = useCallback((e: MouseEvent) => {
    if (!isPlaying) return
    if (!document.pointerLockElement) return

    if (e.button === 0) {
      if (weaponMode.current === 'blue') {
        placePortal('blue')
      } else {
        shoot()
      }
    } else if (e.button === 2) {
      placePortal('orange')
    }
  }, [isPlaying, shoot, placePortal])

  const handleKeyDown = useCallback((e: KeyboardEvent) => {
    if (!isPlaying) return

    if (e.code === 'KeyR') {
      if (!isReloading.current && ammo < 30) {
        isReloading.current = true
        setTimeout(() => {
          reload()
          isReloading.current = false
        }, 1500)
      }
    }
    if (e.code === 'Digit1') weaponMode.current = 'gun'
    if (e.code === 'Digit2') weaponMode.current = 'blue'
    if (e.code === 'Digit3') weaponMode.current = 'orange'
  }, [isPlaying, ammo, reload])

  const handleContextMenu = useCallback((e: Event) => {
    e.preventDefault()
  }, [])

  useEffect(() => {
    document.addEventListener('mousedown', handleMouseDown)
    document.addEventListener('keydown', handleKeyDown)
    document.addEventListener('contextmenu', handleContextMenu)
    return () => {
      document.removeEventListener('mousedown', handleMouseDown)
      document.removeEventListener('keydown', handleKeyDown)
      document.removeEventListener('contextmenu', handleContextMenu)
    }
  }, [handleMouseDown, handleKeyDown, handleContextMenu])

  useFrame((_, delta) => {
    if (hitMarkerTimer.current > 0) {
      hitMarkerTimer.current -= delta
    }
  })

  return null
}
