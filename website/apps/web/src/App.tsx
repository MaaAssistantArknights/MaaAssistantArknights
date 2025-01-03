import { HomeHero } from './components/home/HomeHero'

function App() {
  return (
    <main className="h-full max-h-screen overflow-hidden">
      <section className="h-screen min-h-[20rem] w-full relative">
        <HomeHero />
      </section>
    </main>
  )
}

export default App
