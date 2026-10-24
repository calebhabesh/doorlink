// Every timestamp uses one explicit locale so phones set to 24-hour time or a
// day-first locale show the same 12-hour, month-first values as desktop.
const LOCALE = 'en-US';

type DateInput = string | number | Date;

const timeFormat = new Intl.DateTimeFormat(LOCALE, {
  hour: 'numeric', minute: '2-digit', second: '2-digit', hour12: true,
});
const shortTimeFormat = new Intl.DateTimeFormat(LOCALE, {
  hour: 'numeric', minute: '2-digit', hour12: true,
});
const dateFormat = new Intl.DateTimeFormat(LOCALE, {
  month: '2-digit', day: '2-digit', year: 'numeric',
});
const longDateFormat = new Intl.DateTimeFormat(LOCALE, {
  weekday: 'short', month: 'short', day: 'numeric', year: 'numeric',
});

const toDate = (value: DateInput) => (value instanceof Date ? value : new Date(value));

/** 3:53:33 PM */
export const formatTime = (value: DateInput) => timeFormat.format(toDate(value));

/** 3:53 PM */
export const formatShortTime = (value: DateInput) => shortTimeFormat.format(toDate(value));

/** 10/24/2026 */
export const formatDate = (value: DateInput) => dateFormat.format(toDate(value));

/** Sat, Oct 24, 2026 */
export const formatLongDate = (value: DateInput) => longDateFormat.format(toDate(value));

/** 10/24/2026, 3:53:33 PM */
export const formatDateTime = (value: DateInput) => `${formatDate(value)}, ${formatTime(value)}`;

/** October, or October 2026 with the year */
export const formatMonth = (value: DateInput, options: { year?: boolean; short?: boolean } = {}) =>
  toDate(value).toLocaleString(LOCALE, {
    month: options.short ? 'short' : 'long',
    ...(options.year ? { year: 'numeric' as const } : {}),
  });
