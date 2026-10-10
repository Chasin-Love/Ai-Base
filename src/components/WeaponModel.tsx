import { useRef } from 'react'
import { useFrame, useThree } from '@react-three/fiber'
import * as THREE from 'three'

export function WeaponModel() {
  const { camera } = useThree()
  const groupRef = useRef<THREE.Group>(null)
  const bobRef = useRef(0)

  useFrame((state, delta) => {
    if (!groupRef.current) return

    // Position weapon relative to camera
    const offset = new THREE.Vector3(0.3, -0.25, -0.5)
    offset.applyQuaternion(camera.quaternion)
    groupRef.current.position.copy(camera.position).add(offset)
    groupRef.current.quaternion.copy(camera.quaternion)

    // Subtle bob animation
    bobRef.current += delta * 5
    const bob = Math.sin(bobRef.current) * 0.005
    groupRef.current.position.y += bob
  })

  return (
    <group ref={groupRef}>
      {/* Gun body */}
      <mesh position={[0, 0, 0]}>
        <boxGeometry args={[0.06, 0.08, 0.3]} />
        <meshStandardMaterial color="#333344" metalness={0.8} roughness={0.2} />
      </mesh>
      {/* Barrel */}
      <mesh position={[0, 0.01, -0.2]}>
        <cylinderGeometry args={[0.02, 0.02, 0.15, 8]} />
        <meshStandardMaterial color="#222233" metalness={0.9} roughness={0.1} />
      </mesh>
      {/* Handle */}
      <mesh position={[0, -0.08, 0.05]} rotation={[0.3, 0, 0]}>
        <boxGeometry args={[0.05, 0.1, 0.06]} />
        <meshStandardMaterial color="#444455" metalness={0.5} roughness={0.5} />
      </mesh>
      {/* Portal emitter glow */}
      <mesh position={[0, 0.01, -0.28]}>
        <sphereGeometry args={[0.025, 8, 8]} />
        <meshBasicMaterial color="#4488ff" />
      </mesh>
      <pointLight position={[0, 0.01, -0.28]} color="#4488ff" intensity={0.5} distance={1} />
    </group>
  )
}
