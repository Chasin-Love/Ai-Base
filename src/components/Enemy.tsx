import { useRef } from 'react'
import { useFrame, useThree } from '@react-three/fiber'
import * as THREE from 'three'
import { useGameState } from '../hooks/useGameState'
import type { Enemy as EnemyType } from '../hooks/useGameState'

interface EnemyProps {
  enemy: EnemyType
}

export function Enemy({ enemy }: EnemyProps) {
  const meshRef = useRef<THREE.Group>(null)
  const { camera } = useThree()
  const { updateEnemyPosition, takeDamage } = useGameState()
  const lastAttack = useRef(0)

  useFrame((state, delta) => {
    if (!enemy.isAlive || !meshRef.current) return

    const time = state.clock.elapsedTime
    const playerPos = camera.position.clone()
    playerPos.y = 1
    const enemyPos = new THREE.Vector3(...enemy.position)
    const direction = playerPos.clone().sub(enemyPos).normalize()
    const distance = playerPos.distanceTo(enemyPos)

    // Move toward player
    if (distance > 3) {
      const newPos: [number, number, number] = [
        enemy.position[0] + direction.x * enemy.speed * delta,
        enemy.position[1],
        enemy.position[2] + direction.z * enemy.speed * delta,
      ]
      updateEnemyPosition(enemy.id, newPos)
      meshRef.current.position.set(...newPos)
    } else {
      meshRef.current.position.set(...enemy.position)
      // Attack player when close
      if (time - lastAttack.current > 1.5) {
        lastAttack.current = time
        takeDamage(5)
      }
    }

    // Bobbing animation
    meshRef.current.position.y = enemy.position[1] + Math.sin(time * 3) * 0.1

    // Face player
    meshRef.current.lookAt(playerPos.x, meshRef.current.position.y, playerPos.z)
  })

  if (!enemy.isAlive) return null

  const healthPercent = enemy.health / enemy.maxHealth

  return (
    <group ref={meshRef} position={enemy.position}>
      {/* Body */}
      <mesh castShadow position={[0, 0, 0]}>
        <capsuleGeometry args={[0.4, 0.8, 4, 8]} />
        <meshStandardMaterial color="#ff3355" roughness={0.5} metalness={0.3} />
      </mesh>

      {/* Head */}
      <mesh castShadow position={[0, 0.9, 0]}>
        <sphereGeometry args={[0.3, 8, 8]} />
        <meshStandardMaterial color="#ff5577" roughness={0.4} metalness={0.4} />
      </mesh>

      {/* Eyes */}
      <mesh position={[0.1, 0.95, 0.25]}>
        <sphereGeometry args={[0.06, 8, 8]} />
        <meshBasicMaterial color="#ffff00" />
      </mesh>
      <mesh position={[-0.1, 0.95, 0.25]}>
        <sphereGeometry args={[0.06, 8, 8]} />
        <meshBasicMaterial color="#ffff00" />
      </mesh>

      {/* Health bar */}
      <group position={[0, 1.5, 0]}>
        <mesh>
          <planeGeometry args={[1, 0.1]} />
          <meshBasicMaterial color="#333333" />
        </mesh>
        <mesh position={[(healthPercent - 1) * 0.5, 0, 0.01]}>
          <planeGeometry args={[healthPercent, 0.08]} />
          <meshBasicMaterial color={healthPercent > 0.5 ? '#44ff44' : healthPercent > 0.25 ? '#ffaa00' : '#ff4444'} />
        </mesh>
      </group>

      {/* Glow effect */}
      <pointLight color="#ff3355" intensity={0.5} distance={3} />
    </group>
  )
}
