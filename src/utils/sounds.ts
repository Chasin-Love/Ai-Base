class SoundManager {
  private audioContext: AudioContext | null = null
  private initialized = false

  init() {
    if (this.initialized) return
    try {
      this.audioContext = new (window.AudioContext || (window as any).webkitAudioContext)()
      this.initialized = true
    } catch (e) {
      console.warn('Web Audio API not supported')
    }
  }

  playShoot() {
    if (!this.audioContext) return
    const ctx = this.audioContext
    const osc = ctx.createOscillator()
    const gain = ctx.createGain()
    osc.connect(gain)
    gain.connect(ctx.destination)
    osc.type = 'sawtooth'
    osc.frequency.setValueAtTime(200, ctx.currentTime)
    osc.frequency.exponentialRampToValueAtTime(50, ctx.currentTime + 0.1)
    gain.gain.setValueAtTime(0.3, ctx.currentTime)
    gain.gain.exponentialRampToValueAtTime(0.01, ctx.currentTime + 0.1)
    osc.start(ctx.currentTime)
    osc.stop(ctx.currentTime + 0.1)
  }

  playPortalPlace() {
    if (!this.audioContext) return
    const ctx = this.audioContext
    const osc = ctx.createOscillator()
    const gain = ctx.createGain()
    osc.connect(gain)
    gain.connect(ctx.destination)
    osc.type = 'sine'
    osc.frequency.setValueAtTime(400, ctx.currentTime)
    osc.frequency.exponentialRampToValueAtTime(800, ctx.currentTime + 0.2)
    gain.gain.setValueAtTime(0.2, ctx.currentTime)
    gain.gain.exponentialRampToValueAtTime(0.01, ctx.currentTime + 0.3)
    osc.start(ctx.currentTime)
    osc.stop(ctx.currentTime + 0.3)
  }

  playTeleport() {
    if (!this.audioContext) return
    const ctx = this.audioContext
    const osc = ctx.createOscillator()
    const gain = ctx.createGain()
    osc.connect(gain)
    gain.connect(ctx.destination)
    osc.type = 'sine'
    osc.frequency.setValueAtTime(200, ctx.currentTime)
    osc.frequency.exponentialRampToValueAtTime(1200, ctx.currentTime + 0.15)
    osc.frequency.exponentialRampToValueAtTime(200, ctx.currentTime + 0.3)
    gain.gain.setValueAtTime(0.2, ctx.currentTime)
    gain.gain.exponentialRampToValueAtTime(0.01, ctx.currentTime + 0.3)
    osc.start(ctx.currentTime)
    osc.stop(ctx.currentTime + 0.3)
  }

  playHit() {
    if (!this.audioContext) return
    const ctx = this.audioContext
    const osc = ctx.createOscillator()
    const gain = ctx.createGain()
    osc.connect(gain)
    gain.connect(ctx.destination)
    osc.type = 'square'
    osc.frequency.setValueAtTime(800, ctx.currentTime)
    osc.frequency.exponentialRampToValueAtTime(200, ctx.currentTime + 0.05)
    gain.gain.setValueAtTime(0.15, ctx.currentTime)
    gain.gain.exponentialRampToValueAtTime(0.01, ctx.currentTime + 0.05)
    osc.start(ctx.currentTime)
    osc.stop(ctx.currentTime + 0.05)
  }

  playKill() {
    if (!this.audioContext) return
    const ctx = this.audioContext
    // Two-tone kill sound
    const osc1 = ctx.createOscillator()
    const osc2 = ctx.createOscillator()
    const gain = ctx.createGain()
    osc1.connect(gain)
    osc2.connect(gain)
    gain.connect(ctx.destination)
    osc1.type = 'sine'
    osc2.type = 'sine'
    osc1.frequency.setValueAtTime(523, ctx.currentTime)
    osc2.frequency.setValueAtTime(659, ctx.currentTime + 0.1)
    gain.gain.setValueAtTime(0.2, ctx.currentTime)
    gain.gain.exponentialRampToValueAtTime(0.01, ctx.currentTime + 0.3)
    osc1.start(ctx.currentTime)
    osc1.stop(ctx.currentTime + 0.1)
    osc2.start(ctx.currentTime + 0.1)
    osc2.stop(ctx.currentTime + 0.3)
  }
}

export const soundManager = new SoundManager()
