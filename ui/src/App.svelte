<script>
  import '@/App.scss'
  import Navbar from '@/components/Navbar.svelte';
  import Environment from './pages/Environment.svelte';
  import Effects from './pages/Effects.svelte';
  import Images from './pages/Images.svelte';
  import Text from './pages/Text.svelte';
  import Settings from './pages/Settings.svelte';

  import 'chartjs-adapter-date-fns';
  import 'chart.js/auto';
  // import { Chart as ChartJS, Title, Tooltip, Legend, LineElement, LinearScale, TimeScale, PointElement, CategoryScale } from 'chart.js';
  // ChartJS.register(Title, Tooltip, Legend, TimeScale, LineElement, LinearScale, PointElement, CategoryScale);

  let currentTab = 'environment';
</script>

<Navbar {currentTab} on:tabchange={(e) => currentTab = e.detail} />
<main class="py-5 lg:py-10 py-10 lg:pl-72">
  <div class="fixed inset-0 -z-10 mx-0 max-w-none overflow-hidden">
    <div class="absolute left-1/2 top-0 ml-[-38rem] h-[25rem] w-[81.25rem] dark:[mask-image:linear-gradient(white,transparent)]">
      <div class="absolute inset-0 bg-gradient-to-r from-[#36b49f] to-[#DBFF75] opacity-40 [mask-image:radial-gradient(farthest-side_at_top,white,transparent)] dark:from-[#36b49f]/30 dark:to-[#DBFF75]/30 dark:opacity-100">
        <svg aria-hidden="true" class="absolute inset-x-0 inset-y-[-50%] h-[200%] w-full skew-y-[-18deg] fill-black/40 stroke-black/50 mix-blend-overlay dark:fill-white/2.5 dark:stroke-white/5">
          <defs>
            <pattern id=":S1:" width="72" height="56" patternUnits="userSpaceOnUse" x="-12" y="4">
              <path d="M.5 56V.5H72" fill="none"></path>
            </pattern>
          </defs>
          <rect width="100%" height="100%" stroke-width="0" fill="url(#:S1:)"></rect>
        </svg>
      </div>
    </div>
    <svg viewBox="0 0 1113 440" aria-hidden="true" class="absolute left-1/2 top-0 ml-[-19rem] w-[69.5625rem] fill-white blur-[26px] dark:hidden">
      <path d="M.016 439.5s-9.5-300 434-300S882.516 20 882.516 20V0h230.004v439.5H.016Z"></path>
    </svg>
  </div>
  <div class="px-6 sm:px-6 lg:px-8">
    {#if currentTab === 'environment'}
      <Environment />
    {:else if currentTab === 'effects'}
      <Effects />
    {:else if currentTab === 'images'}
      <Images />
    {:else if currentTab === 'text'}
      <Text />
    {:else if currentTab === 'settings'}
      <Settings />
    {:else}
      <Environment />
    {/if}
  </div>
</main>