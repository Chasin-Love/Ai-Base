import { useState, useEffect, useRef } from 'react'
import { useGameState } from '../hooks/useGameState'

export function HUD() {
  const { health, ammo, maxAmmo, score, kills, isPlaying, enemies } = useGameState()
  const [hitMarker, setHitMarker] = useState(false)
  const [killFeed, setKillFeed] = useState<string[]>([])

  const aliveEnemies = enemies.filter(e => e.isAlive).length

  // Track kills for kill feed
  const prevKillsRef = useRef(kills)
  useEffect(() => {
    if (kills > prevKillsRef.current) {
      setKillFeed(prev => [...prev.slice(-4), `☠ Enemy eliminated! +100`])
      setTimeout(() => {
        setKillFeed(prev => prev.slice(1))
      }, 3000)
    }
    prevKillsRef.current = kills
  }, [kills])

  // Hit marker on damage dealt (simplified - shows on shoot)
  useEffect(() => {
    const handleShoot = () => {
      setHitMarker(true)
      setTimeout(() => setHitMarker(false), 150)
    }
    document.addEventListener('mousedown', handleShoot)
    return () => document.removeEventListener('mousedown', handleShoot)
  }, [])

  if (!isPlaying) return null

  const healthColor = health > 60 ? '#44ff88' : health > 30 ? '#ffaa00' : '#ff4444'
  const healthPercent = health

  return (
    <div className="fixed inset-0 pointer-events-none z-50 select-none">
      {/* Crosshair */}
      <div className="absolute top-1/2 left-1/2 -translate-x-1/2 -translate-y-1/2">
        <div className="relative w-8 h-8">
          <div className={`absolute top-1/2 left-0 w-2.5 h-0.5 -translate-y-1/2 transition-colors duration-100 ${hitMarker ? 'bg-red-400' : 'bg-white/70'}`} />
          <div className={`absolute top-1/2 right-0 w-2.5 h-0.5 -translate-y-1/2 transition-colors duration-100 ${hitMarker ? 'bg-red-400' : 'bg-white/70'}`} />
          <div className={`absolute left-1/2 top-0 w-0.5 h-2.5 -translate-x-1/2 transition-colors duration-100 ${hitMarker ? 'bg-red-400' : 'bg-white/70'}`} />
          <div className={`absolute left-1/2 bottom-0 w-0.5 h-2.5 -translate-x-1/2 transition-colors duration-100 ${hitMarker ? 'bg-red-400' : 'bg-white/70'}`} />
          <div className="absolute top-1/2 left-1/2 w-0.5 h-0.5 bg-white/40 rounded-full -translate-x-1/2 -translate-y-1/2" />
        </div>
      </div>

      {/* Hit marker X */}
      {hitMarker && (
        <div className="absolute top-1/2 left-1/2 -translate-x-1/2 -translate-y-1/2">
          <div className="relative w-6 h-6">
            <div className="absolute top-1/2 left-1/2 w-4 h-0.5 bg-red-400 -translate-x-1/2 -translate-y-1/2 rotate-45" />
            <div className="absolute top-1/2 left-1/2 w-4 h-0.5 bg-red-400 -translate-x-1/2 -translate-y-1/2 -rotate-45" />
          </div>
        </div>
      )}

      {/* Health bar */}
      <div className="absolute bottom-8 left-8">
        <div className="flex items-center gap-3">
          <div className="text-2xl">❤️</div>
          <div>
            <div className="text-xs text-white/60 uppercase tracking-wider mb-1">Health</div>
            <div className="w-48 h-3 bg-black/60 rounded-full border border-white/20 overflow-hidden backdrop-blur-sm">
              <div
                className="h-full rounded-full transition-all duration-300"
                style={{
                  width: `${healthPercent}%`,
                  backgroundColor: healthColor,
                  boxShadow: `0 0 10px ${healthColor}`
                }}
              />
            </div>
            <div className="text-sm text-white/80 mt-1 font-mono">{health}/100</div>
          </div>
        </div>
      </div>

      {/* Ammo */}
      <div className="absolute bottom-8 right-8">
        <div className="text-right">
          <div className="text-xs text-white/60 uppercase tracking-wider mb-1">Ammo</div>
          <div className="flex items-baseline gap-1 justify-end">
            <span className={`text-4xl font-bold font-mono ${ammo === 0 ? 'text-red-400' : 'text-white'}`}>
              {ammo}
            </span>
            <span className="text-lg text-white/50 font-mono">/ {maxAmmo}</span>
          </div>
          <div className="text-xs text-white/40 mt-1">[R] Reload</div>
        </div>
      </div>

      {/* Score & Kills */}
      <div className="absolute top-8 right-8">
        <div className="text-right bg-black/30 backdrop-blur-sm rounded-lg p-3 border border-white/10">
          <div className="text-xs text-white/60 uppercase tracking-wider">Score</div>
          <div className="text-3xl font-bold text-yellow-400 font-mono">{score}</div>
          <div className="text-sm text-white/60 mt-2 border-t border-white/10 pt-2">
            <span className="text-red-400">☠</span> {kills} kills
          </div>
        </div>
      </div>

      {/* Weapon mode indicator */}
      <div className="absolute bottom-8 left-1/2 -translate-x-1/2">
        <div className="flex gap-2 bg-black/40 backdrop-blur-sm rounded-lg p-2 border border-white/10">
          <div className="px-3 py-1 rounded text-xs text-white/60 border border-white/10">
            [1] 🔫 Gun
          </div>
          <div className="px-3 py-1 rounded text-xs text-blue-300 border border-blue-400/30 bg-blue-500/10">
            [2] 🔵 Blue Portal
          </div>
          <div className="px-3 py-1 rounded text-xs text-orange-300 border border-orange-400/30 bg-orange-500/10">
            [3] 🟠 Orange Portal
          </div>
        </div>
      </div>

      {/* Controls hint */}
      <div className="absolute top-8 left-8">
        <div className="text-xs text-white/30 space-y-1 bg-black/20 backdrop-blur-sm rounded p-2">
          <div>WASD - Move</div>
          <div>Mouse - Look</div>
          <div>Space - Jump</div>
          <div>LMB - Shoot / Place Blue</div>
          <div>RMB - Place Orange Portal</div>
        </div>
      </div>

      {/* Portal status */}
      <div className="absolute bottom-24 left-8 flex gap-3">
        <div className="flex items-center gap-2 bg-black/30 rounded px-2 py-1">
          <div className="w-3 h-3 rounded-full bg-blue-500 shadow-lg shadow-blue-500/50 animate-pulse" />
          <span className="text-xs text-blue-300">Blue</span>
        </div>
        <div className="flex items-center gap-2 bg-black/30 rounded px-2 py-1">
          <div className="w-3 h-3 rounded-full bg-orange-500 shadow-lg shadow-orange-500/50 animate-pulse" />
          <span className="text-xs text-orange-300">Orange</span>
        </div>
      </div>

      {/* Enemy count */}
      <div className="absolute top-8 left-1/2 -translate-x-1/2">
        <div className="text-center bg-black/30 backdrop-blur-sm rounded-lg px-4 py-2 border border-white/10">
          <div className="text-xs text-white/40 uppercase tracking-wider">Enemies</div>
          <div className="text-xl font-bold text-red-400 font-mono">{aliveEnemies}</div>
        </div>
      </div>

      {/* Kill feed */}
      <div className="absolute top-24 right-8 space-y-1">
        {killFeed.map((msg, i) => (
          <div
            key={i}
            className="text-sm text-yellow-300 bg-black/40 rounded px-2 py-1 animate-pulse"
          >
            {msg}
          </div>
        ))}
      </div>

      {/* Damage overlay */}
      {health < 30 && (
        <div className="absolute inset-0 border-4 border-red-500/30 rounded-lg animate-pulse pointer-events-none" />
      )}

      {/* Low ammo warning */}
      {ammo <= 5 && ammo > 0 && (
        <div className="absolute bottom-20 right-8 text-xs text-yellow-400 animate-pulse">
          ⚠️ Low ammo!
        </div>
      )}
    </div>
  )
}
