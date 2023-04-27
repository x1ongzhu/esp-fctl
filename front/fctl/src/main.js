/**
 * main.js
 *
 * Bootstraps Vuetify and other plugins then mounts the App`
 */

// Components
import App from './App.vue'

// Composables
import { createApp } from 'vue'

// Plugins
import { registerPlugins } from '@/plugins'
import './main.less'
import axios from 'axios'

axios.defaults.baseURL = import.meta.env.VITE_API_BASE_URL

const app = createApp(App)

registerPlugins(app)

app.mount('#app')
