import { Game } from './components/Game'
import { HUD } from './components/HUD'

function App() {
  return (
    <div className="w-screen h-screen overflow-hidden bg-black relative">
      <Game />
      <HUD />
    </div>
  )
}

export default App
