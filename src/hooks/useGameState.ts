import { create } from 'zustand'
import * as THREE from 'three'

export interface Portal {
  position: [number, number, number]
  normal: [number, number, number]
  id: string
}

export interface Enemy {
  id: string
  position: [number, number, number]
  health: number
  maxHealth: number
  speed: number
  isAlive: boolean
}

interface GameState {
  health: number
  maxHealth: number
  ammo: number
  maxAmmo: number
  score: number
  kills: number
  isPlaying: boolean
  isPaused: boolean
  showMenu: boolean
  bluePortal: Portal | null
  orangePortal: Portal | null
  enemies: Enemy[]

  setHealth: (health: number) => void
  takeDamage: (amount: number) => void
  setAmmo: (ammo: number) => void
  useAmmo: (amount: number) => void
  reload: () => void
  addScore: (points: number) => void
  addKill: () => void
  setBluePortal: (portal: Portal | null) => void
  setOrangePortal: (portal: Portal | null) => void
  setPlaying: (playing: boolean) => void
  setPaused: (paused: boolean) => void
  setShowMenu: (show: boolean) => void
  spawnEnemy: () => void
  damageEnemy: (id: string, damage: number) => void
  updateEnemyPosition: (id: string, position: [number, number, number]) => void
  resetGame: () => void
}

export const useGameState = create<GameState>((set, get) => ({
  health: 100,
  maxHealth: 100,
  ammo: 30,
  maxAmmo: 30,
  score: 0,
  kills: 0,
  isPlaying: false,
  isPaused: false,
  showMenu: true,
  bluePortal: null,
  orangePortal: null,
  enemies: [],

  setHealth: (health: number) => set({ health }),
  takeDamage: (amount: number) => set((state: GameState) => ({
    health: Math.max(0, state.health - amount)
  })),
  setAmmo: (ammo: number) => set({ ammo }),
  useAmmo: (amount: number) => set((state: GameState) => ({
    ammo: Math.max(0, state.ammo - amount)
  })),
  reload: () => set({ ammo: get().maxAmmo }),
  addScore: (points: number) => set((state: GameState) => ({ score: state.score + points })),
  addKill: () => set((state: GameState) => ({ kills: state.kills + 1 })),
  setBluePortal: (portal: Portal | null) => set({ bluePortal: portal }),
  setOrangePortal: (portal: Portal | null) => set({ orangePortal: portal }),
  setPlaying: (playing: boolean) => set({ isPlaying: playing }),
  setPaused: (paused: boolean) => set({ isPaused: paused }),
  setShowMenu: (show: boolean) => set({ showMenu: show }),

  spawnEnemy: () => {
    const positions: [number, number, number][] = [
      [15, 1, 15], [-15, 1, -15], [15, 1, -15], [-15, 1, 15],
      [0, 1, 20], [20, 1, 0], [-20, 1, 0], [0, 1, -20],
      [10, 1, 0], [-10, 1, 0], [0, 1, 10], [0, 1, -10],
    ]
    const pos = positions[Math.floor(Math.random() * positions.length)]
    const enemy: Enemy = {
      id: `enemy-${Date.now()}-${Math.random()}`,
      position: pos,
      health: 100,
      maxHealth: 100,
      speed: 2 + Math.random() * 3,
      isAlive: true,
    }
    set((state: GameState) => ({ enemies: [...state.enemies, enemy] }))
  },

  damageEnemy: (id: string, damage: number) => {
    const state = get()
    const enemies = state.enemies.map((e: Enemy) => {
      if (e.id === id) {
        const newHealth = e.health - damage
        return { ...e, health: newHealth, isAlive: newHealth > 0 }
      }
      return e
    })
    set({ enemies })
  },

  updateEnemyPosition: (id: string, position: [number, number, number]) => {
    set((state: GameState) => ({
      enemies: state.enemies.map((e: Enemy) =>
        e.id === id ? { ...e, position } : e
      )
    }))
  },

  resetGame: () => set({
    health: 100,
    ammo: 30,
    score: 0,
    kills: 0,
    enemies: [],
    bluePortal: null,
    orangePortal: null,
    isPlaying: true,
    isPaused: false,
    showMenu: false,
  }),
}))
