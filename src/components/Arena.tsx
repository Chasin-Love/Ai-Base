import { useRef } from 'react'
import * as THREE from 'three'

export function Arena() {
  const wallMaterial = useRef(
    new THREE.MeshStandardMaterial({ color: '#2a2a3e', roughness: 0.8 })
  )
  const floorMaterial = useRef(
    new THREE.MeshStandardMaterial({ color: '#1a1a2e', roughness: 0.9, metalness: 0.1 })
  )
  const platformMaterial = useRef(
    new THREE.MeshStandardMaterial({ color: '#3a3a5e', roughness: 0.6, metalness: 0.3 })
  )
  const pillarMaterial = useRef(
    new THREE.MeshStandardMaterial({ color: '#4a3a6e', roughness: 0.5, metalness: 0.4 })
  )

  return (
    <group>
      {/* Floor */}
      <mesh rotation={[-Math.PI / 2, 0, 0]} position={[0, 0, 0]} receiveShadow>
        <planeGeometry args={[60, 60]} />
        <meshStandardMaterial color="#1a1a2e" roughness={0.9} metalness={0.1} />
      </mesh>

      {/* Floor grid lines */}
      <gridHelper args={[60, 30, '#333355', '#222244']} position={[0, 0.01, 0]} />

      {/* Walls */}
      {/* North wall */}
      <mesh position={[0, 4, -30]} castShadow receiveShadow>
        <boxGeometry args={[60, 8, 1]} />
        <meshStandardMaterial color="#2a2a3e" roughness={0.8} />
      </mesh>
      {/* South wall */}
      <mesh position={[0, 4, 30]} castShadow receiveShadow>
        <boxGeometry args={[60, 8, 1]} />
        <meshStandardMaterial color="#2a2a3e" roughness={0.8} />
      </mesh>
      {/* East wall */}
      <mesh position={[30, 4, 0]} castShadow receiveShadow>
        <boxGeometry args={[1, 8, 60]} />
        <meshStandardMaterial color="#2a2a3e" roughness={0.8} />
      </mesh>
      {/* West wall */}
      <mesh position={[-30, 4, 0]} castShadow receiveShadow>
        <boxGeometry args={[1, 8, 60]} />
        <meshStandardMaterial color="#2a2a3e" roughness={0.8} />
      </mesh>

      {/* Central platform */}
      <mesh position={[0, 1.5, 0]} castShadow receiveShadow>
        <boxGeometry args={[10, 0.5, 10]} />
        <meshStandardMaterial color="#3a3a5e" roughness={0.6} metalness={0.3} />
      </mesh>
      {/* Central ramp */}
      <mesh position={[0, 0.75, 6]} rotation={[0.3, 0, 0]} castShadow receiveShadow>
        <boxGeometry args={[4, 0.3, 4]} />
        <meshStandardMaterial color="#3a3a5e" roughness={0.6} metalness={0.3} />
      </mesh>

      {/* Corner platforms */}
      <mesh position={[13, 1, 13]} castShadow receiveShadow>
        <boxGeometry args={[6, 0.5, 6]} />
        <meshStandardMaterial color="#3a3a5e" roughness={0.6} metalness={0.3} />
      </mesh>
      <mesh position={[-13, 1, -13]} castShadow receiveShadow>
        <boxGeometry args={[6, 0.5, 6]} />
        <meshStandardMaterial color="#3a3a5e" roughness={0.6} metalness={0.3} />
      </mesh>
      <mesh position={[13, 1, -13]} castShadow receiveShadow>
        <boxGeometry args={[6, 0.5, 6]} />
        <meshStandardMaterial color="#3a3a5e" roughness={0.6} metalness={0.3} />
      </mesh>
      <mesh position={[-13, 1, 13]} castShadow receiveShadow>
        <boxGeometry args={[6, 0.5, 6]} />
        <meshStandardMaterial color="#3a3a5e" roughness={0.6} metalness={0.3} />
      </mesh>

      {/* Pillars */}
      {[
        [8, 3, 0], [-8, 3, 0], [0, 3, 8], [0, 3, -8],
        [20, 3, 20], [-20, 3, -20], [20, 3, -20], [-20, 3, 20],
      ].map((pos, i) => (
        <mesh key={`pillar-${i}`} position={pos as [number, number, number]} castShadow receiveShadow>
          <cylinderGeometry args={[0.5, 0.5, 6, 8]} />
          <meshStandardMaterial color="#4a3a6e" roughness={0.5} metalness={0.4} />
        </mesh>
      ))}

      {/* Central structure - cross walls */}
      <mesh position={[0, 2, 0]} castShadow receiveShadow>
        <boxGeometry args={[6, 4, 0.5]} />
        <meshStandardMaterial color="#2a2a4e" roughness={0.7} />
      </mesh>
      <mesh position={[0, 2, 0]} castShadow receiveShadow>
        <boxGeometry args={[0.5, 4, 6]} />
        <meshStandardMaterial color="#2a2a4e" roughness={0.7} />
      </mesh>

      {/* Side barriers */}
      <mesh position={[15, 1.5, 0]} castShadow receiveShadow>
        <boxGeometry args={[1, 3, 6]} />
        <meshStandardMaterial color="#2a2a4e" roughness={0.7} />
      </mesh>
      <mesh position={[-15, 1.5, 0]} castShadow receiveShadow>
        <boxGeometry args={[1, 3, 6]} />
        <meshStandardMaterial color="#2a2a4e" roughness={0.7} />
      </mesh>
      <mesh position={[0, 1.5, 15]} castShadow receiveShadow>
        <boxGeometry args={[6, 3, 1]} />
        <meshStandardMaterial color="#2a2a4e" roughness={0.7} />
      </mesh>
      <mesh position={[0, 1.5, -15]} castShadow receiveShadow>
        <boxGeometry args={[6, 3, 1]} />
        <meshStandardMaterial color="#2a2a4e" roughness={0.7} />
      </mesh>

      {/* Lighting */}
      <ambientLight intensity={0.3} />
      <directionalLight
        position={[10, 20, 10]}
        intensity={0.8}
        castShadow
        shadow-mapSize={[2048, 2048]}
      />
      <pointLight position={[0, 8, 0]} intensity={1} color="#6644ff" distance={30} />
      <pointLight position={[20, 5, 20]} intensity={0.5} color="#ff4466" distance={20} />
      <pointLight position={[-20, 5, -20]} intensity={0.5} color="#44ff66" distance={20} />
      <pointLight position={[20, 5, -20]} intensity={0.5} color="#4466ff" distance={20} />
      <pointLight position={[-20, 5, 20]} intensity={0.5} color="#ffaa44" distance={20} />

      {/* Neon accents on floor */}
      <mesh position={[0, 0.02, 0]} rotation={[-Math.PI / 2, 0, 0]}>
        <ringGeometry args={[4.5, 5, 32]} />
        <meshBasicMaterial color="#6644ff" transparent opacity={0.5} />
      </mesh>
      <mesh position={[0, 0.02, 0]} rotation={[-Math.PI / 2, 0, 0]}>
        <ringGeometry args={[9, 9.3, 32]} />
        <meshBasicMaterial color="#4466ff" transparent opacity={0.3} />
      </mesh>
    </group>
  )
}
