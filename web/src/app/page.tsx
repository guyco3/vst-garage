import catalog from '../../public/data/catalog.json';

type Plugin = {
  id: string;
  name: string;
  version: string;
  description: string;
  tags: string[];
  mac_url: string;
  win_url: string;
};

const plugins = catalog as Plugin[];

function PrismArt({ flip }: { flip?: boolean }) {
  return (
    <div
      className={`relative h-64 overflow-hidden rounded-[1.8rem] bg-gradient-to-br from-fuchsia-500/25 via-sky-500/25 to-violet-500/25 ${
        flip ? 'md:rotate-1' : 'md:-rotate-1'
      }`}
      aria-hidden="true"
    >
      <div className="absolute -left-10 top-8 h-40 w-40 rotate-12 bg-gradient-to-br from-fuchsia-500/40 to-transparent blur-2xl" />
      <div className="absolute right-2 top-4 h-44 w-44 -rotate-6 bg-gradient-to-br from-sky-500/40 to-transparent blur-2xl" />
      <div className="absolute -bottom-8 left-20 h-44 w-44 rotate-45 bg-gradient-to-br from-violet-500/40 to-transparent blur-2xl" />

      <svg className="absolute inset-0 h-full w-full" viewBox="0 0 100 100" preserveAspectRatio="none">
        <polygon points="5,90 45,20 78,78" className="fill-white/35" />
        <polygon points="30,88 66,30 95,95" className="fill-white/25" />
        <polygon points="0,100 28,56 47,100" className="fill-white/40" />
      </svg>
    </div>
  );
}

export default function HomePage() {

  return (
    <main className="relative mx-auto min-h-screen w-full max-w-6xl overflow-hidden px-6 py-12">
      <div className="pointer-events-none absolute inset-0 -z-10">
        <div className="absolute -left-28 top-28 h-72 w-72 rotate-12 bg-fuchsia-500/28 blur-3xl" />
        <div className="absolute right-[-90px] top-[26rem] h-80 w-80 -rotate-12 bg-sky-500/30 blur-3xl" />
        <div className="absolute left-1/3 top-[58rem] h-72 w-72 rotate-45 bg-violet-500/26 blur-3xl" />

        <div
          className="absolute -left-24 top-[22rem] h-[22rem] w-[16rem] rotate-[22deg] bg-gradient-to-br from-violet-500/32 to-fuchsia-500/14"
          style={{ clipPath: 'polygon(50% 0%, 100% 100%, 0% 100%)' }}
        />
        <div
          className="absolute right-8 top-[52rem] h-[24rem] w-[18rem] -rotate-[16deg] bg-gradient-to-br from-sky-500/30 to-cyan-500/14"
          style={{ clipPath: 'polygon(48% 0%, 100% 100%, 0% 100%)' }}
        />
      </div>

      <header className="mb-16 py-4 md:py-8">
        <p className="mb-3 text-xs uppercase tracking-[0.28em] text-violet-300">VST Garage</p>
        <h1 className="max-w-5xl text-4xl font-semibold tracking-tight text-slate-100 md:text-5xl">
          Welcome to my site.
        </h1>
        <p className="mt-5 max-w-4xl text-lg leading-8 text-slate-300">
          I&apos;m Guy. I&apos;m a software engineer who likes producing music and playing guitar.
        </p>
        <p className="mt-3 max-w-4xl text-base leading-7 text-slate-400">
          Here&apos;s a free list of VSTs I made for people to try out.
        </p>
      </header>

      <section className="space-y-14 md:space-y-20">
        {plugins.map((plugin, index) => (
          <article key={plugin.id} className="py-2">
            <div className={`grid items-center gap-8 md:grid-cols-2 ${index % 2 === 1 ? 'md:[&>*:first-child]:order-2' : ''}`}>
              <div>
                <p className="mb-2 text-xs uppercase tracking-[0.22em] text-slate-400">Plugin {String(index + 1).padStart(2, '0')}</p>
                <div className="mb-3 flex items-center gap-3">
                  <h2 className="text-2xl font-semibold text-slate-100">{plugin.name}</h2>
                  <span className="rounded-md border border-slate-700 bg-slate-900/70 px-2 py-1 text-xs text-slate-300">
                    v{plugin.version}
                  </span>
                </div>

                <p className="text-base leading-7 text-slate-300">{plugin.description}</p>

                <div className="mt-4 flex flex-wrap gap-2">
                  {plugin.tags.map((tag) => (
                    <span key={tag} className="rounded-full border border-violet-400/35 bg-violet-500/10 px-2.5 py-1 text-xs text-violet-200">
                      {tag}
                    </span>
                  ))}
                </div>

                <div className="mt-6 flex gap-3">
                  <a
                    href={plugin.win_url}
                    className="rounded-lg border border-slate-600 bg-slate-900/80 px-4 py-2 text-sm font-medium text-slate-200 hover:bg-slate-800"
                    target="_blank"
                    rel="noreferrer"
                  >
                    Download for Windows
                  </a>
                  <a
                    href={plugin.mac_url}
                    className="rounded-lg bg-gradient-to-r from-violet-500 to-sky-500 px-4 py-2 text-sm font-medium text-white hover:from-violet-400 hover:to-sky-400"
                    target="_blank"
                    rel="noreferrer"
                  >
                    Download for macOS
                  </a>
                </div>
              </div>

              <PrismArt flip={index % 2 === 1} />
            </div>
          </article>
        ))}
      </section>
    </main>
  );
}
