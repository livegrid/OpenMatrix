<script>
    export let message = '';
    export let onClose = () => {};
  </script>
  
  <div class="fixed inset-0 bg-black bg-opacity-50 flex items-center justify-center z-50">
    <div class="bg-white dark:bg-zinc-800 p-6 rounded-lg shadow-xl max-w-md w-full">
      <p class="text-zinc-800 dark:text-zinc-200 mb-4">{message}</p>
      <button
        on:click={onClose}
        class="px-4 py-2 bg-zinc-700 hover:bg-zinc-600 text-zinc-200 rounded-md transition-colors duration-200 ease-in-out focus:outline-none focus:ring-2 focus:ring-zinc-500 focus:ring-opacity-50"
      >
        Close
      </button>
    </div>
  </div>