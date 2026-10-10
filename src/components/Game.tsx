import { useEffect, useRef, useCallback, useState } from 'react'
import { Canvas, useThree, useFrame } from '@react-three/fiber'
import * as THREE from 'three'
import { Player } from './Player'
import { Arena } from './Arena'
import { PortalVisual } from './Portal'
import { Enemy } from './Enemy'
import { Weapon } from './Weapon'
import { WeaponModel } from './WeaponModel'
import { useGameState } from '../hooks/useGameState'
import { soundManager } from '../utils/sounds'

function EnemySpawner() {
  const { spawnEnemy, enemies, isPlaying } = useGameState()
  const lastSpawn = useRef(0)

  useFrame((state) => {
    if (!isPlaying) return
    const time = state.clock.elapsedTime
    const aliveEnemies = enemies.filter(e => e.isAlive).length

    if (time - lastSpawn.current > 3.5 && aliveEnemies < 8) {
      lastSpawn.current = time
      spawnEnemy()
    }
  })

  return null
}

function CameraSetup() {
  const { camera } = useThree()
  const { isPlaying } = useGameState()

  useEffect(() => {
    if (isPlaying) {
      camera.position.set(0, 1.7, 20)
      camera.lookAt(0, 1.7, 0)
    }
  }, [isPlaying, camera])

  return null
}

function Scene() {
  const { bluePortal, orangePortal, enemies } = useGameState()

  return (
    <>
      <CameraSetup />
      <Player />
      <Weapon />
      <WeaponModel />
      <Arena />

      {/* Portals */}
      {bluePortal && <PortalVisual portal={bluePortal} color="blue" />}
      {orangePortal && <PortalVisual portal={orangePortal} color="orange" />}

      {/* Enemies */}
      {enemies.filter(e => e.isAlive).map(enemy => (
        <Enemy key={enemy.id} enemy={enemy} />
      ))}

      <EnemySpawner />

      {/* Environment */}
      <fog attach="fog" args={['#0a0a1a', 25, 55]} />
    </>
  )
}

export function Game() {
  const { isPlaying, showMenu, resetGame, health, setShowMenu, setPlaying, score, kills } = useGameState()
  const [gameOver, setGameOver] = useState(false)

  const handleStart = useCallback(() => {
    soundManager.init()
    resetGame()
    setGameOver(false)
    setTimeout(() => {
      const canvas = document.querySelector('canvas')
      if (canvas) {
        canvas.requestPointerLock()
      }
    }, 100)
  }, [resetGame])

  const handleCanvasClick = useCallback(() => {
    if (isPlaying && !document.pointerLockElement) {
      const canvas = document.querySelector('canvas')
      if (canvas) {
        canvas.requestPointerLock()
      }
    }
  }, [isPlaying])

  // Game over check
  useEffect(() => {
    if (isPlaying && health <= 0) {
      setPlaying(false)
      setShowMenu(true)
      setGameOver(true)
      document.exitPointerLock()
    }
  }, [health, isPlaying, setPlaying, setShowMenu])

  return (
    <div className="w-full h-full" onClick={handleCanvasClick}>
      <Canvas
        shadows
        camera={{ fov: 75, near: 0.1, far: 100 }}
        gl={{ antialias: true, alpha: false }}
      >
        <color attach="background" args={['#0a0a1a']} />
        <Scene />
      </Canvas>

      {/* Menu overlay */}
      {showMenu && (
        <div className="absolute inset-0 flex items-center justify-center bg-black/85 backdrop-blur-md z-50">
          <div className="text-center max-w-lg">
            {/* Logo */}
            <div className="mb-6">
              <h1 className="text-7xl font-black mb-2 tracking-tight">
                <span className="bg-gradient-to-r from-blue-400 via-purple-400 to-orange-400 bg-clip-text text-transparent">
                  PORTAL
                </span>
                <span className="text-white">STRIKE</span>
              </h1>
              <div className="h-1 w-32 mx-auto bg-gradient-to-r from-blue-500 via-purple-500 to-orange-500 rounded-full" />
            </div>

            <p className="text-white/50 text-lg mb-8">
              Arena Shooter with Portal Mechanics
            </p>

            {/* Game Over Stats */}
            {gameOver && (
              <div className="mb-8 bg-white/5 rounded-xl p-6 border border-white/10">
                <div className="text-3xl text-red-400 font-bold mb-4">💀 GAME OVER</div>
                <div className="grid grid-cols-2 gap-4">
                  <div className="bg-black/30 rounded-lg p-3">
                    <div className="text-xs text-white/40 uppercase">Final Score</div>
                    <div className="text-2xl font-bold text-yellow-400 font-mono">{score}</div>
                  </div>
                  <div className="bg-black/30 rounded-lg p-3">
                    <div className="text-xs text-white/40 uppercase">Kills</div>
                    <div className="text-2xl font-bold text-red-400 font-mono">{kills}</div>
                  </div>
                </div>
              </div>
            )}

            {/* Start Button */}
            <button
              onClick={handleStart}
              className="group relative px-10 py-4 bg-gradient-to-r from-blue-600 to-purple-600 hover:from-blue-500 hover:to-purple-500 text-white font-bold text-xl rounded-xl transition-all duration-300 transform hover:scale-105 shadow-lg shadow-purple-500/30 hover:shadow-purple-500/50"
            >
              <span className="relative z-10">
                {gameOver ? '🔄 PLAY AGAIN' : '🎮 START GAME'}
              </span>
              <div className="absolute inset-0 rounded-xl bg-gradient-to-r from-blue-400 to-purple-400 opacity-0 group-hover:opacity-20 transition-opacity" />
            </button>

            {/* Controls */}
            <div className="mt-10 grid grid-cols-2 gap-3 text-left">
              <div className="bg-white/5 rounded-lg p-3 border border-white/10">
                <div className="text-sm font-bold text-white/80 mb-1">🎮 Movement</div>
                <div className="text-xs text-white/40 space-y-0.5">
                  <div>WASD - Move around</div>
                  <div>Space - Jump</div>
                  <div>Mouse - Look around</div>
                </div>
              </div>
              <div className="bg-white/5 rounded-lg p-3 border border-white/10">
                <div className="text-sm font-bold text-white/80 mb-1">⚔️ Combat</div>
                <div className="text-xs text-white/40 space-y-0.5">
                  <div>Left Click - Shoot</div>
                  <div>R - Reload</div>
                  <div>1/2/3 - Switch mode</div>
                </div>
              </div>
              <div className="col-span-2 bg-gradient-to-r from-blue-500/10 to-orange-500/10 rounded-lg p-3 border border-purple-400/20">
                <div className="text-sm font-bold text-purple-300 mb-1">🌀 Portal Mechanics</div>
                <div className="text-xs text-white/50">
                  Place Blue (LMB + mode 2) and Orange (RMB) portals on surfaces. Walk through them to teleport instantly! Use portals to outmaneuver enemies.
                </div>
              </div>
            </div>
          </div>
        </div>
      )}
    </div>
  )
}
