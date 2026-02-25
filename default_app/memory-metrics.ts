import { app } from 'electron/main';

const FOUR_HOURS_MS = 4 * 60 * 60 * 1000;

export interface MemoryMetricData {
  timestamp: number;
  totalWorkingSetSize: number;
  totalPeakWorkingSetSize: number;
  processes: Array<{
    pid: number;
    type: string;
    name?: string;
    workingSetSize: number;
    peakWorkingSetSize: number;
  }>;
}

export type MetricReporter = (data: MemoryMetricData) => void;

let memoryMetricsTimer: ReturnType<typeof setInterval> | null = null;
let metricReporter: MetricReporter | null = null;

/**
 * Collects memory metrics from all Electron processes using app.getAppMetrics()
 */
export function collectMemoryMetrics(): MemoryMetricData {
  const metrics = app.getAppMetrics();

  let totalWorkingSetSize = 0;
  let totalPeakWorkingSetSize = 0;

  const processes = metrics.map(metric => {
    const workingSetSize = metric.memory?.workingSetSize ?? 0;
    const peakWorkingSetSize = metric.memory?.peakWorkingSetSize ?? 0;

    totalWorkingSetSize += workingSetSize;
    totalPeakWorkingSetSize += peakWorkingSetSize;

    return {
      pid: metric.pid,
      type: metric.type,
      name: metric.name,
      workingSetSize,
      peakWorkingSetSize
    };
  });

  return {
    timestamp: Date.now(),
    totalWorkingSetSize,
    totalPeakWorkingSetSize,
    processes
  };
}

/**
 * Starts the memory metrics timer that reports every 4 hours
 * @param reporter - Callback function to handle the collected metrics
 * @param intervalMs - Optional custom interval (defaults to 4 hours)
 */
export function startMemoryMetricsTimer(
  reporter: MetricReporter,
  intervalMs: number = FOUR_HOURS_MS
): void {
  if (memoryMetricsTimer) {
    console.warn('Memory metrics timer is already running');
    return;
  }

  metricReporter = reporter;

  // Collect initial metrics immediately
  const initialMetrics = collectMemoryMetrics();
  metricReporter(initialMetrics);

  // Set up recurring timer
  memoryMetricsTimer = setInterval(() => {
    if (metricReporter) {
      const metrics = collectMemoryMetrics();
      metricReporter(metrics);
    }
  }, intervalMs);

  // Ensure timer doesn't prevent app from exiting
  memoryMetricsTimer.unref();
}

/**
 * Stops the memory metrics timer
 */
export function stopMemoryMetricsTimer(): void {
  if (memoryMetricsTimer) {
    clearInterval(memoryMetricsTimer);
    memoryMetricsTimer = null;
    metricReporter = null;
  }
}

/**
 * Default reporter that logs to console (for development/debugging)
 */
export function consoleMetricReporter(data: MemoryMetricData): void {
  const totalMB = (data.totalWorkingSetSize / 1024).toFixed(2);
  const peakMB = (data.totalPeakWorkingSetSize / 1024).toFixed(2);

  console.log(`[Memory Metrics] ${new Date(data.timestamp).toISOString()}`);
  console.log(`  Total Working Set: ${totalMB} MB`);
  console.log(`  Peak Working Set: ${peakMB} MB`);
  console.log(`  Processes: ${data.processes.length}`);

  for (const proc of data.processes) {
    const procMB = (proc.workingSetSize / 1024).toFixed(2);
    console.log(`    - ${proc.type}${proc.name ? ` (${proc.name})` : ''} [PID ${proc.pid}]: ${procMB} MB`);
  }
}
