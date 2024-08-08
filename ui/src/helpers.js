export async function fetchWithTimeout(resource, options = {}) {
  const { timeout = 8000, retries = 2, retryDelay = 1000 } = options;

  const fetchAttempt = async (attempt) => {
      const controller = new AbortController();
      const id = setTimeout(() => controller.abort(), timeout);
      
      try {
          const response = await fetch(resource, {
              ...options,
              signal: controller.signal
          });
          clearTimeout(id);

          if (!response.ok && attempt < retries) {
              throw new Error(`Fetch error: ${response.statusText}`);
          }

          return response;
      } catch (error) {
          clearTimeout(id);
          if (attempt < retries) {
              console.warn(`Attempt ${attempt + 1} failed: ${error.message}. Retrying in ${retryDelay}ms...`);
              await new Promise(resolve => setTimeout(resolve, retryDelay));
              return fetchAttempt(attempt + 1);
          } else {
              throw error;
          }
      }
  };

  return fetchAttempt(0);
}