<script>
  import { createEventDispatcher, afterUpdate } from 'svelte';
  import Logo from '@/components/Logo.svelte';
  import NavItem from '@/components/NavItem.svelte';
  import NavBottomSettings from '@/components/NavBottomSettings.svelte';
  import { state } from '@/store';

  export let currentTab;
  const dispatch = createEventDispatcher();

  // opening, open, closing, closed
  let navbarState = 'closed';

  const loadTab = () => {
    const tab =location.hash.substring(1);
    dispatch('tabchange', tab  == '' ? 'aquarium' : tab);
  }

  const changeTab = (tab) => {
    if(tab == 'environment') {
        history.pushState(null, null, ' ');
        loadTab();
    } else {
        location.hash = tab;
    }
  }

  const openNavbar = () => {
    console.log('openNavbar');
    navbarState = 'opening';
    setTimeout(() => {
      navbarState = 'open';
    }, 300);
  }

  const closeNavbar = () => {
    console.log('closeNavbar');
    navbarState = 'closing';
    setTimeout(() => {
      navbarState = 'closed';
    }, 300);
  }

  let navMenu = [
    {
      id: 'aquarium',
      modeId: 0,
      title: 'Aquarium',
      icon: 'fish',
      disabled: false,
    },
    {
      id: 'effects',
      modeId: 1,
      title: 'Effects',
      icon: 'sparkles',
      disabled: false,
    },
    {
      id: 'images',
      modeId: 2,
      title: 'Images',
      icon: 'image',
      disabled: false,
    },
    {
      id: 'text',
      modeId: 3,
      title: 'Text',
      icon: 'text',
      disabled: false,
    },
    {
      id: 'settings',
      title: 'Settings',
      icon: 'settings',
      disabled: false,
    },
  ];
  
  window.addEventListener('hashchange', loadTab);
  afterUpdate(() => { loadTab(); });
</script>

<!-- Off-canvas menu for mobile, show/hide based on off-canvas menu navbarState. -->
<div class="relative z-50 lg:hidden" role="dialog" aria-modal="true">
  <!--
    Off-canvas menu backdrop, show/hide based on off-canvas menu navbarState.

    Entering: "transition-opacity ease-linear duration-300"
      From: "opacity-0"
      To: "opacity-100"
    Leaving: "transition-opacity ease-linear duration-300"
      From: "opacity-100"
      To: "opacity-0"
  -->
  {#if navbarState !== 'closed'}

    <div class="fixed inset-0 flex">
      <div on:click={closeNavbar} class={`fixed inset-0 bg-gray-900/80 dark:bg-zinc-900/40 transition-opacity ease-linear duration-300 ${ navbarState === 'open' ? 'opacity-100' : 'opacity-0'}`} />
    <!--
        Off-canvas menu, show/hide based on off-canvas menu navbarState.

        Entering: "transition ease-in-out duration-300 transform"
          From: "-translate-x-full"
          To: "translate-x-0"
        Leaving: "transition ease-in-out duration-300 transform"
          From: "translate-x-0"
          To: "-translate-x-full"
      -->
      <div class={`relative mr-16 flex w-full max-w-xs flex-1 transition ease-in-out duration-300 transform ${ navbarState === 'open' ? 'translate-x-0' : '-translate-x-full' }`}>
        <!--
          Close button, show/hide based on off-canvas menu navbarState.

          Entering: "ease-in-out duration-300"
            From: "opacity-0"
            To: "opacity-100"
          Leaving: "ease-in-out duration-300"
            From: "opacity-100"
            To: "opacity-0"
        -->
        {#if navbarState != 'closed'}
          <div class={`absolute left-full top-0 flex w-16 justify-center pt-5 ease-in-out duration-300 ${ navbarState === 'open' ? 'opacity-100' : 'opacity-0' }`}>
            <button on:click={closeNavbar} class="-m-2.5 p-2.5">
              <span class="sr-only">Close sidebar</span>
              <svg class="h-6 w-6 text-white" fill="none" viewBox="0 0 24 24" stroke-width="1.5" stroke="currentColor" aria-hidden="true">
                <path stroke-linecap="round" stroke-linejoin="round" d="M6 18L18 6M6 6l12 12" />
              </svg>
            </button>
          </div>
        {/if}

        <!-- Sidebar component, swap this element with another sidebar if you like -->
        <div class="flex grow flex-col gap-y-5 overflow-y-auto bg-white dark:bg-black px-6 pb-2">
          <div class="flex h-16 mt-2 shrink-0 items-center">
            <Logo className="h-12 w-auto" />
          </div>
          <nav class="flex flex-1 flex-col">
            <ul class="flex flex-1 flex-col gap-y-7">
              <li>
                <ul class="-mx-2 space-y-1">
                  {#each navMenu as item (item.id)}
                    <li>
                      <NavItem on:click={() => changeTab(item.id)} modeId={item.modeId} active={currentTab === item.id} disabled={Object.keys($state).length === 0 || item.disabled} title={item.title} icon={item.icon} />
                    </li>
                  {/each}
                </ul>
              </li>
              <li>
                <NavBottomSettings />
              </li>
            </ul>
          </nav>
        </div>
      </div>
    </div>
  {/if}
</div>

<!-- Static sidebar for desktop -->
<div class="hidden lg:fixed lg:inset-y-0 lg:z-50 lg:flex lg:w-72 lg:flex-col">
  <!-- Sidebar component, swap this element with another sidebar if you like -->
  <div class="flex grow flex-col gap-y-5 overflow-y-auto border-r border-gray-200 dark:border-zinc-900 bg-white/30 dark:bg-black/30 px-6">
    <div class="flex h-16 mt-2 shrink-0 items-center">
      <Logo className="h-12 w-auto" />
    </div>
    <nav class="flex flex-1 flex-col">
      <ul class="flex flex-1 flex-col gap-y-7">
        <li>
          <ul class="-mx-2 space-y-1">
            {#each navMenu as item (item.id)}
              <li>
                <NavItem on:click={() => changeTab(item.id)} modeId={item.modeId} id={item.id} active={currentTab === item.id} disabled={Object.keys($state).length === 0 || item.disabled} title={item.title} icon={item.icon} />
              </li>
            {/each}
          </ul>
        </li>
        <li class="mt-auto">
          <NavBottomSettings />
        </li>
      </ul>
    </nav>
  </div>
</div>


<div class="sticky top-0 z-40 flex items-center gap-x-6 bg-zinc-100/90 dark:bg-zinc-950/80 lg:bg-transparent border-b border-gray-200 dark:border-zinc-900/90 px-4 py-4 shadow-sm sm:px-6 lg:hidden">
  <Logo className="h-8 w-auto" />
  <div class="flex-1 text-sm font-semibold leading-6 text-gray-900 dark:text-zinc-200"></div>
  <button on:click={openNavbar} class="-m-1 p-2.5 text-gray-700 dark:text-zinc-400 lg:hidden">
    <svg class="h-6 w-6" fill="none" viewBox="0 0 24 24" stroke-width="1.5" stroke="currentColor" aria-hidden="true">
      <path stroke-linecap="round" stroke-linejoin="round" d="M3.75 6.75h16.5M3.75 12h16.5m-16.5 5.25h16.5" />
    </svg>
  </button>
  <!-- <button>
    <span class="sr-only">Settings</span>
    <svg class="h-5 w-5 shrink-0 text-gray-600 dark:text-zinc-600 group-hover:text-emerald-600" xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M20 7h-9"/><path d="M14 17H5"/><circle cx="17" cy="17" r="3"/><circle cx="7" cy="7" r="3"/></svg>
  </button> -->
</div>