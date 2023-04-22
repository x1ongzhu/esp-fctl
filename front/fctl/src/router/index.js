// Composables
import { createRouter, createWebHistory } from 'vue-router'
import Home from '@/views/Home.vue'
import Settings from '@/views/Settings.vue'

const routes = [
    {
        path: '/',
        component: Home
    },
    {
        path: '/settings',
        component: Settings
    }
]

const router = createRouter({
    history: createWebHistory(import.meta.env.BASE_URL),
    routes
})

export default router
