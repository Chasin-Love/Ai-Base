import { useRef, useMemo } from 'react'
import { useFrame } from '@react-three/fiber'
import * as THREE from 'three'
import type { Portal as PortalType } from '../hooks/useGameState'

interface PortalProps {
  portal: PortalType
  color: 'blue' | 'orange'
}

export function PortalVisual({ portal, color }: PortalProps) {
  const ringRef = useRef<THREE.Mesh>(null)
  const glowRef = useRef<THREE.Mesh>(null)
  const innerRef = useRef<THREE.Mesh>(null)

  const portalColor = color === 'blue' ? '#4488ff' : '#ff8844'
  const glowColor = color === 'blue' ? '#2244aa' : '#aa4422'

  const particlePositions = useMemo(() => {
    const arr = new Float32Array(60)
    for (let i = 0; i < 20; i++) {
      const angle = (i / 20) * Math.PI * 2
      arr[i * 3] = Math.cos(angle) * 1.3
      arr[i * 3 + 1] = Math.sin(angle) * 1.3
      arr[i * 3 + 2] = 0
    }
    return arr
  }, [])

  useFrame((state) => {
    const time = state.clock.elapsedTime
    if (ringRef.current) {
      ringRef.current.rotation.z = time * 2
    }
    if (glowRef.current) {
      const scale = 1 + Math.sin(time * 3) * 0.1
      glowRef.current.scale.set(scale, scale, 1)
    }
    if (innerRef.current) {
      const mat = innerRef.current.material as THREE.MeshBasicMaterial
      mat.opacity = 0.3 + Math.sin(time * 4) * 0.15
    }
  })

  const normal = new THREE.Vector3(...portal.normal)
  const quaternion = new THREE.Quaternion()
  quaternion.setFromUnitVectors(new THREE.Vector3(0, 0, 1), normal)
  const rotation = new THREE.Euler().setFromQuaternion(quaternion)

  return (
    <group position={portal.position} rotation={rotation}>
      {/* Outer ring */}
      <mesh ref={ringRef}>
        <torusGeometry args={[1.2, 0.08, 8, 32]} />
        <meshBasicMaterial color={portalColor} />
      </mesh>

      {/* Second ring */}
      <mesh rotation={[0, 0, Math.PI / 4]}>
        <torusGeometry args={[1.0, 0.04, 8, 32]} />
        <meshBasicMaterial color={portalColor} transparent opacity={0.6} />
      </mesh>

      {/* Inner glow */}
      <mesh ref={glowRef}>
        <circleGeometry args={[1.1, 32]} />
        <meshBasicMaterial
          color={glowColor}
          transparent
          opacity={0.6}
          side={THREE.DoubleSide}
        />
      </mesh>

      {/* Portal surface */}
      <mesh ref={innerRef} position={[0, 0, 0.01]}>
        <circleGeometry args={[0.95, 32]} />
        <meshBasicMaterial
          color={portalColor}
          transparent
          opacity={0.4}
          side={THREE.DoubleSide}
        />
      </mesh>

      {/* Particle ring */}
      <points>
        <bufferGeometry>
          <bufferAttribute
            attach="attributes-position"
            count={20}
            array={particlePositions}
            itemSize={3}
          />
        </bufferGeometry>
        <pointsMaterial color={portalColor} size={0.12} transparent opacity={0.8} />
      </points>

      {/* Point light for glow effect */}
      <pointLight color={portalColor} intensity={3} distance={6} />
    </group>
  )
}
