import { Card, CardHeader, CardTitle } from '@/components/Card';
import { useStore } from '@/lib/store';
import { 
  Github, 
  Heart, 
  ExternalLink,
  Code,
  Cpu,
} from 'lucide-react';

export function About() {
  const esp32 = useStore((s) => s.esp32);
  const pico = useStore((s) => s.pico);

  return (
    <div className="space-y-6">
      {/* Hero */}
      <Card className="text-center bg-gradient-to-br from-coffee-800 to-coffee-900 text-white border-0">
        <div className="py-8">
          <img 
            src="/logo.png" 
            alt="BrewOS" 
            className="h-20 mx-auto mb-6 brightness-0 invert"
          />
          <p className="text-cream-300 mb-4">
            Open-source espresso machine controller
          </p>
          <div className="flex items-center justify-center gap-4 text-sm text-cream-400">
            <span>ESP32: {esp32.version || 'Unknown'}</span>
            <span>•</span>
            <span>Pico: {pico.version || 'Unknown'}</span>
          </div>
        </div>
      </Card>

      {/* Features */}
      <div className="grid grid-cols-1 md:grid-cols-3 gap-4">
        <FeatureCard
          icon={<Cpu className="w-6 h-6" />}
          title="Dual-Core Control"
          description="ESP32-S3 for connectivity, Pico for real-time control"
        />
        <FeatureCard
          icon={<Code className="w-6 h-6" />}
          title="Fully Open Source"
          description="Hardware and software designs available on GitHub"
        />
        <FeatureCard
          icon={<Heart className="w-6 h-6" />}
          title="Community Driven"
          description="Built by and for the espresso community"
        />
      </div>

      {/* Links */}
      <Card>
        <CardHeader>
          <CardTitle>Resources</CardTitle>
        </CardHeader>

        <div className="grid grid-cols-1 sm:grid-cols-2 gap-4">
          <LinkCard
            icon={<Github className="w-5 h-5" />}
            title="GitHub Repository"
            description="Source code and documentation"
            href="https://github.com/your-org/brewos"
          />
          <LinkCard
            icon={<ExternalLink className="w-5 h-5" />}
            title="Documentation"
            description="Setup guides and API reference"
            href="https://brewos.dev/docs"
          />
        </div>
      </Card>

      {/* System Info */}
      <Card>
        <CardHeader>
          <CardTitle>System Information</CardTitle>
        </CardHeader>

        <div className="grid grid-cols-2 sm:grid-cols-4 gap-4 text-sm">
          <div>
            <span className="text-coffee-500">ESP32 Version</span>
            <p className="font-mono font-medium text-coffee-900">{esp32.version || '—'}</p>
          </div>
          <div>
            <span className="text-coffee-500">Pico Version</span>
            <p className="font-mono font-medium text-coffee-900">{pico.version || '—'}</p>
          </div>
          <div>
            <span className="text-coffee-500">UI Version</span>
            <p className="font-mono font-medium text-coffee-900">1.0.0</p>
          </div>
          <div>
            <span className="text-coffee-500">Build</span>
            <p className="font-mono font-medium text-coffee-900">
              {import.meta.env.MODE === 'production' ? 'Production' : 'Development'}
            </p>
          </div>
        </div>
      </Card>

      {/* Credits */}
      <Card className="text-center">
        <p className="text-coffee-500 mb-2">
          Made with <Heart className="w-4 h-4 inline text-red-500" /> for espresso lovers
        </p>
        <p className="text-sm text-coffee-400">
          © {new Date().getFullYear()} BrewOS Contributors
        </p>
      </Card>
    </div>
  );
}

interface FeatureCardProps {
  icon: React.ReactNode;
  title: string;
  description: string;
}

function FeatureCard({ icon, title, description }: FeatureCardProps) {
  return (
    <Card className="text-center">
      <div className="w-12 h-12 bg-accent/10 rounded-xl flex items-center justify-center mx-auto mb-3 text-accent">
        {icon}
      </div>
      <h3 className="font-semibold text-coffee-900 mb-1">{title}</h3>
      <p className="text-sm text-coffee-500">{description}</p>
    </Card>
  );
}

interface LinkCardProps {
  icon: React.ReactNode;
  title: string;
  description: string;
  href: string;
}

function LinkCard({ icon, title, description, href }: LinkCardProps) {
  return (
    <a
      href={href}
      target="_blank"
      rel="noopener noreferrer"
      className="flex items-start gap-4 p-4 bg-cream-100 rounded-xl hover:bg-cream-200 transition-colors group"
    >
      <div className="w-10 h-10 bg-white rounded-lg flex items-center justify-center text-coffee-600 group-hover:text-accent transition-colors">
        {icon}
      </div>
      <div>
        <h4 className="font-semibold text-coffee-900 group-hover:text-accent transition-colors">
          {title}
        </h4>
        <p className="text-sm text-coffee-500">{description}</p>
      </div>
    </a>
  );
}

